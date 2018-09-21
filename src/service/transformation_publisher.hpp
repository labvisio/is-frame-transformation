#pragma once

#include <is/msgs/camera.pb.h>
#include <chrono>
#include <is/wire/core.hpp>
#include <string>
#include <vector>
#include "dependency_tracker.hpp"

namespace is {

class TransformationPublisher {
  Channel channel;
  std::shared_ptr<opentracing::Tracer> tracer;
  DependencyTracker* tracker;
  FrameConversion* conversions;

  // Used to throttle message publication
  std::chrono::system_clock::time_point publish_deadline;
  std::unordered_map<std::string, vision::FrameTransformation> transformations;

  // create topic from ids
  auto create_topic(is::Path const& ids) -> std::string;
  auto next_deadline() -> std::chrono::system_clock::time_point;

 public:
  TransformationPublisher(Channel const&, Subscription*,
                          std::shared_ptr<opentracing::Tracer> const&, DependencyTracker*,
                          FrameConversion*);

  auto run(boost::optional<Message> const&) -> std::chrono::system_clock::time_point;
};

}  // namespace is