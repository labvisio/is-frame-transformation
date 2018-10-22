#include "transformation-publisher.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <regex>

namespace is {

auto TransformationPublisher::create_topic(is::Path const& path) -> std::string {
  auto joined = boost::algorithm::join(
      path | boost::adaptors::transformed([](int64_t i) { return std::to_string(i); }), ".");
  return "FrameTransformation." + joined;
}

static constexpr auto throttle_interval = std::chrono::milliseconds(100);

TransformationPublisher::TransformationPublisher(Channel const& ch, Subscription* sub,
                                                 std::shared_ptr<opentracing::Tracer> const& trace,
                                                 DependencyTracker* track, FrameConversion* conv)
    : channel(ch),
      tracer(trace),
      tracker(track),
      conversions(conv),
      publish_deadline(std::chrono::system_clock::now() + throttle_interval) {
  sub->subscribe("#.FrameTransformations");
}

auto TransformationPublisher::next_deadline() -> std::chrono::system_clock::time_point {
  return transformations.empty() ? std::chrono::system_clock::now() + std::chrono::seconds(10)
                                 : publish_deadline;
}

auto TransformationPublisher::run(boost::optional<Message> const& msg)
    -> std::chrono::system_clock::time_point {
  if (msg) {
    if (!std::regex_match(msg->topic(), std::regex(".+\\.FrameTransformations"))) {
      return next_deadline();
    }

    auto maybe_ctx = msg->extract_tracing(tracer);
    auto root = maybe_ctx ? tracer->StartSpan("UpdateTFs", {opentracing::ChildOf(maybe_ctx->get())})
                          : tracer->StartSpan("UpdateTFs");

    auto tfs = msg->unpack<vision::FrameTransformations>();
    if (!tfs) {
      is::warn("event=Publisher.BadSchema");
      return next_deadline();
    }

    for (auto&& tf : tfs->tfs()) {
      // Recompute transformation using the graphs calculated earlier
      // is::info("event=Publisher.Update from={} to={}", tf.from(), tf.to());
      tracker->update(tf, [&](is::Path const& path, vision::FrameTransformation const& new_tf) {
        auto topic = create_topic(path);
        transformations[topic] = new_tf;
      });
    }

    if (tfs->tfs_size() == 0) {
      /* If there is no tf we check if the message comes from a dynamic source. Dynamic sources are
       * transformations that come from a detection process that can fail. The empty tf indicates
       * that the proccess failed. Therefore all the poses related to that source should be removed
       * otherwise we will compute new tfs using outdated values.
       */
      auto topic = msg->topic();
      auto match = std::smatch{};
      if (std::regex_match(topic, match, std::regex("ArUco.(\\d+).FrameTransformations"))) {
        auto id = std::stoi(match[1]);
        // Filter transformations that go from the camera id to any id on the dynamic range
        auto edges_to_remove = std::vector<Edge>{};
        for (auto&& edge_and_tensor : conversions->transformations()) {
          auto edge = edge_and_tensor.first;
          if (edge.from == id && (edge.to >= 100 && edge.to <= 150)) {
            edges_to_remove.push_back(edge);
          }
        }

        // Remove those transformations
        for (auto&& edge : edges_to_remove) {
          tracker->invalidate_edge(edge);
          conversions->remove_transformation(edge);
        }
      }
    }
  }

  auto now = std::chrono::system_clock::now();
  if (now >= next_deadline()) {
    for (auto&& key_val : transformations) {
      auto topic = key_val.first;
      auto transformation = key_val.second;
      channel.publish(topic, Message{transformation});
      // is::info("event=Publisher.Pub from={} to={}", transformation.from(), transformation.to());
    }
    transformations.clear();
    publish_deadline = now + throttle_interval;
  }

  return next_deadline();
}

}  // namespace is
