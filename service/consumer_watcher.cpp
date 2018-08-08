#include "consumer_watcher.hpp"
#include <algorithm>
#include <iterator>
#include "is/wire/core/logger.hpp"

namespace is {

ConsumerWatcher::ConsumerWatcher(Subscription* subscription) {
  subscription->subscribe("BrokerEvents.Consumers");
}

void ConsumerWatcher::on_new_consumer(
    std::function<void(std::string const&, std::string const&)> const& callback) {
  _new_consumer = callback;
}

void ConsumerWatcher::on_no_consumers(std::function<void(std::string const&)> const& callback) {
  _no_consumers = callback;
}

auto ConsumerWatcher::run(Message const& msg) -> bool {
  if (msg.topic() != "BrokerEvents.Consumers") return false;
  auto new_info = msg.unpack<common::ConsumerList>()->info();

  using ConsumerVector = std::vector<std::pair<std::string, common::ConsumerInfo>>;

  // Converts Map: Topic -> ConsumerInfo to vector
  auto new_consumers = ConsumerVector{};
  new_consumers.reserve(new_info.size());
  std::copy(new_info.cbegin(), new_info.cend(), std::back_inserter(new_consumers));

  // Filter only the topics we are interested using the regexp
  auto re = std::regex{"FrameTransformation(?:\\.\\d+){2,}"};
  new_consumers.erase(
      std::remove_if(new_consumers.begin(), new_consumers.end(),
                     [&](auto const& key_pair) { return !std::regex_match(key_pair.first, re); }),
      new_consumers.end());

  std::sort(new_consumers.begin(), new_consumers.end(),
            [](auto const& l, auto const& r) { return l.first < r.first; });

  auto create_topic_projection = [](ConsumerVector const& from) {
    auto topics = std::vector<std::string>{};
    topics.reserve(from.size());
    std::transform(from.begin(), from.end(), std::back_inserter(topics),
                   [](auto const& el) { return el.first; });
    return topics;
  };

  auto new_topics = create_topic_projection(new_consumers);
  auto old_topics = create_topic_projection(_consumers);

  auto deleted = std::vector<std::string>{};
  std::set_difference(old_topics.begin(), old_topics.end(), new_topics.begin(), new_topics.end(),
                      std::inserter(deleted, deleted.begin()));
  for (auto const& topic : deleted) {
    is::info("[ConsumerWatcher][-] topic='{}'", topic);
    _no_consumers(topic);
  }

  auto created = std::vector<std::string>{};
  std::set_difference(new_topics.begin(), new_topics.end(), old_topics.begin(), old_topics.end(),
                      std::inserter(created, created.begin()));
  for (auto const& topic : created) {
    is::info("[ConsumerWatcher][+] topic='{}'", topic);
    _new_consumer(topic, topic);
  }

  auto intersection = std::vector<std::string>{};
  std::set_intersection(new_topics.begin(), new_topics.end(), old_topics.begin(), old_topics.end(),
                        std::back_inserter(intersection));

  for (auto const& topic : intersection) {
    auto comparator = [](auto const& l, auto const& r) { return l.first < r; };
    auto old = std::lower_bound(_consumers.begin(), _consumers.end(), topic, comparator);
    auto now = std::lower_bound(new_consumers.begin(), new_consumers.end(), topic, comparator);
    assert(now != new_consumers.end() && old != _consumers.end());

    auto oldc = old->second.mutable_consumers();
    std::sort(oldc->begin(), oldc->end());

    auto newc = now->second.mutable_consumers();
    std::sort(newc->begin(), newc->end());

    auto added = std::vector<std::string>{};
    std::set_difference(newc->begin(), newc->end(), oldc->begin(), oldc->end(),
                        std::inserter(added, added.begin()));

    for (auto const& consumer : added) {
      is::info("[ConsumerWatcher][+] topic='{}' consumer='{}'", topic, consumer);
      _new_consumer(topic, consumer);
    }
  }
  _consumers = new_consumers;
  return true;
}

}  // namespace is
