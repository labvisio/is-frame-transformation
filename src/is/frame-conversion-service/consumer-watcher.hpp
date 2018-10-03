#pragma once

#include <is/msgs/common.pb.h>
#include <functional>
#include <is/wire/core.hpp>
#include <regex>
#include <string>
#include <vector>

namespace is {

/* Watches BrokerEvents for new/no consumers on topics with the FrameTransformation.<IDs...>
 * pattern*/
class ConsumerWatcher {
  std::vector<std::pair<std::string, common::ConsumerInfo>> consumers;
  //  (path, consumer) -> void
  std::function<void(std::string const&, std::string const&)> new_consumer;
  //  (path) -> void
  std::function<void(std::string const&)> no_consumers;

 public:
  ConsumerWatcher(Subscription*);
  void on_new_consumer(std::function<void(std::string const&, std::string const&)> const& callback);
  void on_no_consumers(std::function<void(std::string const&)> const& callback);
  void run(Message const&);
};

}  // namespace is