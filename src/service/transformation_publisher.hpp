#pragma once

#include <string>
#include <vector>
#include "dependency_tracker.hpp"
#include "is/wire/core.hpp"
#include "is/msgs/camera.pb.h"

namespace is {

class TransformationPublisher {
  Channel _channel;
  std::shared_ptr<opentracing::Tracer> _tracer;
  DependencyTracker* _tracker;
  FrameConversion* _conversions;

  // Used to throttle message publication
  std::chrono::system_clock::time_point _publish_deadline;
  std::unordered_map<std::string, vision::FrameTransformation> _tensors;

  // create topic from ids
  auto create_topic(is::Path const& ids) -> std::string;

 public:
  TransformationPublisher(Channel const&, Subscription*,
                          std::shared_ptr<opentracing::Tracer> const& tracer, DependencyTracker*,
                          FrameConversion*);

  auto run(Message const&) -> bool;
};

}  // namespace is