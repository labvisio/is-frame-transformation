#pragma once

#include <string>
#include <vector>
#include "dependency_tracker.hpp"
#include "is/wire/core.hpp"

namespace is {

class TransformationPublisher {
  Channel _channel;
  std::shared_ptr<opentracing::Tracer> _tracer;
  DependencyTracker* _tracker;
  FrameConversion* _conversions;

  // Used to throttle message publication
  static constexpr auto throttle_interval = std::chrono::milliseconds(100);
  std::unordered_map<std::string, common::Tensor> _tensors;
  std::chrono::system_clock::time_point _publish_deadline =
      std::chrono::system_clock::now() + throttle_interval;

  // create topic from ids
  auto create_topic(is::Path const& ids) -> std::string;

 public:
  TransformationPublisher(Channel const&, Subscription*,
                          std::shared_ptr<opentracing::Tracer> const& tracer, DependencyTracker*,
                          FrameConversion*);

  auto run(Message const&) -> bool;
};

}  // namespace is