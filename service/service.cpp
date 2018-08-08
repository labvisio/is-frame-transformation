#include "boost/algorithm/string.hpp"
#include "calibration_server.hpp"
#include "consumer_watcher.hpp"
#include "dependency_tracker.hpp"
#include "frame_conversion/frame_conversion.hpp"
#include "is/wire/core.hpp"
#include "is/wire/rpc.hpp"
#include "is/wire/rpc/log-interceptor.hpp"
#include "transformation_publisher.hpp"
#include "utils.hpp"

int main(int argc, char* argv[]) {
  auto const service = std::string{"FrameConversion"};

  auto options = load_cli_options(argc, argv);

  auto tracer = make_tracer(service, options.zipkin_uri());

  auto channel = is::Channel{options.broker_uri()};
  channel.set_tracer(tracer);

  auto subscription = is::Subscription{channel};

  auto server = is::ServiceProvider{channel};
  auto logs = is::LogInterceptor{};
  server.add_interceptor(logs);

  auto calibs = is::CalibrationServer{options.calibrations_path()};

  server.delegate<is::vision::GetCalibrationRequest, is::vision::GetCalibrationReply>(
      service + ".GetCalibration", [&](auto* ctx, auto const& request, auto* reply) {
        return calibs.get_calibration(ctx, request, reply);
      });

  auto conversions = is::FrameConversion{};
  for (auto const& calibration : calibs.calibrations()) {
    for (auto const& transformation : calibration.second.extrinsic()) {
      conversions.update_transformation(transformation);
    }
  }

  auto tracker = is::DependencyTracker{&conversions};  // conversions must outlive the tracker

  // Watch consumers of this service and updates the dependency tracker.
  auto watcher = is::ConsumerWatcher{&subscription};

  // Make Path from topic string.
  auto create_path = [](std::string const& from) {
    std::vector<std::string> ids;
    boost::split(ids, from, boost::is_any_of("."));
    is::Path path;
    path.reserve(ids.size() - 1);
    std::transform(ids.begin() + 1, ids.end(), std::back_inserter(path),
                   [](std::string const& id) { return std::stoll(id); });
    return path;
  };

  watcher.on_new_consumer([&](std::string const& topic, std::string const& consumer) {
    auto maybe_tf = tracker.update_dependency(create_path(topic));
    if (maybe_tf) { channel.publish(consumer, is::Message{*maybe_tf}); }
  });

  watcher.on_no_consumers(
      [&](std::string const& topic) { tracker.remove_dependency(create_path(topic)); });

  auto transformation_publisher =
      is::TransformationPublisher{channel, &subscription, tracer, &tracker};

  for (;;) {
    auto message = channel.consume();
    auto ok = transformation_publisher.run(message);
    if (!ok) ok = watcher.run(message);
    if (!ok) ok = server.serve(message);
  }
}
