#include <regex>
#include "consumer_watcher.hpp"
#include "dependency_tracker.hpp"
#include "frame_conversion/frame_conversion.hpp"
#include "is/msgs/camera.pb.h"
#include "is/wire/core.hpp"
#include "is/wire/rpc.hpp"
#include "is/wire/rpc/log-interceptor.hpp"
#include "utils.hpp"

namespace vis = is::vision;

int main(int argc, char* argv[]) {
  auto options = load_cli_options(argc, argv);
  auto channel = is::Channel{options.broker_uri()};

  is::FrameConversion conversions;
  std::unordered_map<int64_t, vis::CameraCalibration> calibrations;
  for (auto const& calibration : load_calibrations(options.calibrations_path())) {
    for (auto const& transformation : calibration.extrinsic()) { conversions.add(transformation); }
    calibrations[calibration.id()] = calibration;
    is::info("[+] {}", calibration);
  }

  std::string service = "FrameConversion";

  auto server = is::ServiceProvider{channel};
  is::LogInterceptor logs{};
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

  server.delegate<vis::GetTransformationRequest, vis::GetTransformationReply>(
      service + ".GetTransformation",
      [&conversions](is::Context*, vis::GetTransformationRequest const& request,
                     vis::GetTransformationReply* reply) {
        for (auto&& id : request.ids()) {
          auto maybe_tf = conversions.find(id.from(), id.to());
          if (!maybe_tf)
            return is::make_status(is::wire::StatusCode::FAILED_PRECONDITION, maybe_tf.error());

          auto transformation = reply->add_transformations();
          transformation->set_from(id.from());
          transformation->set_to(id.to());
          *(transformation->mutable_tf()) = maybe_tf->tf;
        }
        return is::make_status(is::wire::StatusCode::OK);
      });

  is::Subscription subscription{channel};
  is::ConsumerWatcher watcher{subscription};
  is::DependencyTracker tracker{&conversions};  // conversions must outlive the tracker

  auto on_consumers = [&](std::smatch const& args) {
    auto from = std::stoll(args[1]);
    auto to = std::stoll(args[2]);
    auto maybe_tf = tracker.add_dependency(is::FrameIds{from, to});
    if (maybe_tf) { channel.publish(args[0], is::Message{maybe_tf->tf}); }
  };

  auto on_no_consumers = [&](std::smatch const& args) {
    auto from = std::stoll(args[1]);
    auto to = std::stoll(args[2]);
    tracker.remove_dependency(is::FrameIds{from, to});
  };

  watcher.on(std::regex{"FrameTransformation.(\\d+).(\\d+)"}, on_consumers, on_no_consumers);

  subscription.subscribe("#.Pose");
  auto update_transformations = [&](is::Message const& message) -> bool {
    if (std::regex_match(message.topic(), std::regex(".+\\.Pose"))) {
      auto tf = message.unpack<is::vision::FrameTransformation>();
      if (tf) {
        // Recompute transformation using the graphs calculated earlier
        // TODO: Recompute the path to search for a possible new better path
        tracker.update(*tf, [&](is::vision::FrameTransformation const& new_tf) {
          is::info("[NewPose] Updating {}->{} that depends on {}<->{}", new_tf.from(), new_tf.to(),
                   tf->from(), tf->to());
          channel.publish(fmt::format("FrameTransformation.{}.{}", new_tf.from(), new_tf.to()),
                          is::Message{new_tf.tf()});
        });
      } else {
        is::warn("[NewPose] Received message with wrong schema");
      }
      return true;
    }
    return false;
  };

  for (;;) {
    auto message = channel.consume();
    auto ok = update_transformations(message);
    if (!ok) ok = watcher.eval(message);
    if (!ok) ok = server.serve(message);
  }
}
