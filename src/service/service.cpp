#include <zipkin/opentracing.h>
#include <boost/algorithm/string.hpp>
#include <is/msgs/utils.hpp>
#include <is/wire/core.hpp>
#include <is/wire/rpc.hpp>
#include <is/wire/rpc/log-interceptor.hpp>
#include "calibration_server.hpp"
#include "conf/options.pb.h"
#include "consumer_watcher.hpp"
#include "dependency_tracker.hpp"
#include "frame_conversion/frame_conversion.hpp"
#include "transformation_publisher.hpp"

auto load_configuration(int argc, char** argv) -> is::FrameConversionServiceOptions {
  auto filename = (argc == 2) ? argv[1] : "options.json";
  auto options = is::FrameConversionServiceOptions{};
  is::load(filename, &options);
  is::validate_message(options);
  return options;
}

auto create_tracer(std::string const& name, std::string const& uri)
    -> std::shared_ptr<opentracing::Tracer> {
  std::smatch match;
  auto ok = std::regex_match(uri, match, std::regex("http:\\/\\/([a-zA-Z0-9\\.]+)(:(\\d+))?"));
  if (!ok) is::critical("Invalid zipkin uri \"{}\", expected http://<hostname>:<port>", uri);
  auto tracer_options = zipkin::ZipkinOtTracerOptions{};
  tracer_options.service_name = name;
  tracer_options.collector_host = match[1];
  tracer_options.collector_port = match[3].length() ? std::stoi(match[3]) : 9411;
  return zipkin::makeZipkinOtTracer(tracer_options);
}

int main(int argc, char* argv[]) {
  auto const service = std::string{"FrameTransformation"};

  auto options = load_configuration(argc, argv);
  auto tracer = create_tracer(service, options.zipkin_uri());

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

  auto tracker = is::DependencyTracker{&conversions};

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
    auto path = create_path(topic);
    auto maybe_transformation = tracker.update_dependency(path);
    if (maybe_transformation) { channel.publish(consumer, is::Message{*maybe_transformation}); }
  });

  watcher.on_no_consumers(
      [&](std::string const& topic) { tracker.remove_dependency(create_path(topic)); });

  auto transformation_publisher =
      is::TransformationPublisher{channel, &subscription, tracer, &tracker, &conversions};

  auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(1);
  for (;;) {
    auto maybe_message = channel.consume_until(deadline);
    deadline = transformation_publisher.run(maybe_message);
    if (maybe_message) {
      watcher.run(*maybe_message);
      server.serve(*maybe_message);
    }
  }
}
