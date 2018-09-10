#pragma once

#include <functional>
#include <regex>
#include <string>
#include <vector>
#include "is/msgs/common.pb.h"
#include "is/wire/core.hpp"

namespace is {

/* Watches BrokerEvents for new/no consumers on topics with the FrameTransformation.<IDs...>
 * pattern*/
class ConsumerWatcher {
  std::vector<std::pair<std::string, common::ConsumerInfo>> _consumers;
  //  (path, consumer) -> void
  std::function<void(std::string const&, std::string const&)> _new_consumer;
  //  (path) -> void
  std::function<void(std::string const&)> _no_consumers;

 public:
  ConsumerWatcher(Subscription*);
  void on_new_consumer(std::function<void(std::string const&, std::string const&)> const& callback);
  void on_no_consumers(std::function<void(std::string const&)> const& callback);
  auto run(Message const&) -> bool;
};

}  // namespace is