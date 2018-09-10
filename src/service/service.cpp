#include <zipkin/opentracing.h>
#include <backward.hpp>
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

namespace backward {
backward::SignalHandling sh;
}  // namespace backward

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
    auto maybe_tf = tracker.update_dependency(create_path(topic));
    if (maybe_tf) { channel.publish(consumer, is::Message{*maybe_tf}); }
  });

  watcher.on_no_consumers(
      [&](std::string const& topic) { tracker.remove_dependency(create_path(topic)); });

  auto transformation_publisher =
      is::TransformationPublisher{channel, &subscription, tracer, &tracker, &conversions};

  for (;;) {
    auto message = channel.consume();
    auto ok = transformation_publisher.run(message);
    if (!ok) ok = watcher.run(message);
    if (!ok) ok = server.serve(message);
  }
}
