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

  // create topic from ids
  auto create_topic(is::Path const& ids) -> std::string;

 public:
  TransformationPublisher(Channel const& channel, Subscription* subscription,
                          std::shared_ptr<opentracing::Tracer> const& tracer,
                          DependencyTracker* tracker);

  auto run(Message const&) -> bool;
};

}  // namespace is