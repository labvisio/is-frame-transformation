#include "consumer_watcher.hpp"
#include "is/wire/core/logger.hpp"

namespace is {

ConsumerWatcher::ConsumerWatcher(Subscription& s) {
  s.subscribe("binding.*");
}

void ConsumerWatcher::on(std::regex const& re, Callback const& newc, Callback const& noc) {
  expression = re;
  new_consumer = newc;
  no_consumers = noc;
}

auto ConsumerWatcher::eval(Message const& msg) -> bool {
  auto const topic = msg.topic();
  if (std::regex_match(topic, std::regex("binding.(created|deleted)"))) {
    auto const binding = msg.metadata().at("routing_key");
    std::smatch matches;
    if (std::regex_match(binding, matches, expression)) {
      auto& ref_count = ref_counts[binding];
      if (topic[8] == 'c') {
        if (ref_count == 0) {
          info("[ConsumerWatcher] [+] \"{}\"", matches[0]);
          new_consumer(matches);
        }
        ++ref_count;
      } else {
        --ref_count;
        if (ref_count == 0) {
          info("[ConsumerWatcher] [-] \"{}\"", matches[0]);
          no_consumers(matches);
        }
        if (ref_count < 0) ref_count = 0;
      }
      return true;
    }
  }
  return false;
}
}