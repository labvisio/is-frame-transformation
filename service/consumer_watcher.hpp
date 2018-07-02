#pragma once

#include <functional>
#include <regex>
#include <unordered_map>
#include "is/wire/core.hpp"

namespace is {

class ConsumerWatcher {
  using Callback = std::function<void(std::smatch const&)>;

  std::regex expression;
  std::unordered_map<std::string, int> ref_counts;
  Callback new_consumer;
  Callback no_consumers;

 public:
  ConsumerWatcher(Subscription&);

  void on(std::regex const&, Callback const& new_consumer, Callback const& no_consumers);
  auto eval(Message const&) -> bool;
};

}