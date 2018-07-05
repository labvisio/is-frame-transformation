#include <regex>
#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/compare.hpp"
#include "boost/range/adaptor/transformed.hpp"
#include "consumer_watcher.hpp"
#include "dependency_tracker.hpp"
#include "frame_conversion/frame_conversion.hpp"
#include "is/msgs/camera.pb.h"
#include "is/wire/core.hpp"
#include "is/wire/rpc.hpp"
#include "is/wire/rpc/log-interceptor.hpp"
#include "utils.hpp"

namespace vis = is::vision;

auto join_ids(std::vector<int64_t> const& ids) -> std::string {
  return fmt::format(
      "FrameTransformation.{}",
      boost::algorithm::join(
          ids | boost::adaptors::transformed([](int64_t i) { return std::to_string(i); }), "."));
}

auto update_transformations(is::Message const& message, is::Channel const& channel,
                            is::DependencyTracker& tracker,
                            std::shared_ptr<opentracing::Tracer> const& tracer) -> bool {
  if (std::regex_match(message.topic(), std::regex(".+\\.Pose"))) {
    auto maybe_ctx = message.extract_tracing(tracer);
    auto root = maybe_ctx ? tracer->StartSpan("UpdateTFs", {opentracing::ChildOf(maybe_ctx->get())})
                          : tracer->StartSpan("UpdateTFs");

    auto tf = message.unpack<is::vision::FrameTransformation>();
    if (tf) {
      // Recompute transformation using the graphs calculated earlier
      // TODO: Recompute the path to search for a possible new better path
      tracker.update(*tf, [&](is::Path const& path, is::common::Tensor const& new_tf) {
        auto span = tracer->StartSpan("NewTF", {opentracing::ChildOf(&root->context())});
        auto topic = join_ids(path);
        auto reply = is::Message{new_tf};
        reply.inject_tracing(tracer, span->context());
        is::info("[NewPose] Updating {}, which depends on {}<->{}", topic, tf->from(), tf->to());
        channel.publish(topic, reply);
      });
    } else {
      is::warn("[NewPose] Wrong Schema");
    }
    return true;
  }
  return false;
}

int main(int argc, char* argv[]) {
  auto const service = std::string{"FrameConversion"};

  auto options = load_cli_options(argc, argv);

  auto conversions = is::FrameConversion{};
  auto calibrations = std::unordered_map<int64_t, vis::CameraCalibration>{};
  for (auto const& calibration : load_calibrations(options.calibrations_path())) {
    for (auto const& transformation : calibration.extrinsic()) {
      conversions.update_transformation(transformation);
    }
    calibrations[calibration.id()] = calibration;
    is::info("[+] {}", calibration.DebugString());
  }

  auto tracer = make_tracer(service, options.zipkin_uri());

  auto channel = is::Channel{options.broker_uri()};
  channel.set_tracer(tracer);

  auto server = is::ServiceProvider{channel};
  auto logs = is::LogInterceptor{};
  server.add_interceptor(logs);

  server.delegate<vis::GetCalibrationRequest, vis::GetCalibrationReply>(
      service + ".GetCalibration",
      [&calibrations](is::Context*, vis::GetCalibrationRequest const& request,
                      vis::GetCalibrationReply* reply) {
        for (auto&& id : request.ids()) {
          auto it = calibrations.find(id);
          if (it == calibrations.end())
            return is::make_status(is::wire::StatusCode::NOT_FOUND,
                                   fmt::format("CameraCalibration with id \"{}\" not found", id));
          *reply->add_calibrations() = it->second;
        }
        return is::make_status(is::wire::StatusCode::OK);
      });

  auto subscription = is::Subscription{channel};
  auto watcher = is::ConsumerWatcher{subscription};
  auto tracker = is::DependencyTracker{&conversions};  // conversions must outlive the tracker

  auto parse_path = [](std::string const& topic) {
    std::vector<std::string> ids;
    boost::split(ids, topic, boost::is_any_of("."));
    is::Path path;
    path.reserve(ids.size() - 1);
    std::transform(ids.begin() + 1, ids.end(), std::back_inserter(path),
                   [](std::string const& id) { return std::stoll(id); });
    return path;
  };

  auto on_consumers = [&](std::smatch const& args) {
    auto maybe_tf = tracker.update_dependency(parse_path(args[0]));
    if (maybe_tf) { channel.publish(args[0], is::Message{*maybe_tf}); }
  };

  auto on_no_consumers = [&](std::smatch const& args) {
    tracker.remove_dependency(parse_path(args[0]));
  };

  watcher.on(std::regex{"FrameTransformation(?:\\.\\d+){2,}"}, on_consumers, on_no_consumers);
  subscription.subscribe("#.Pose");

  for (;;) {
    auto message = channel.consume();
    auto ok = update_transformations(message, channel, tracker, tracer);
    if (!ok) ok = watcher.eval(message);
    if (!ok) ok = server.serve(message);
  }
}
