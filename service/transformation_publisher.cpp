#include "transformation_publisher.hpp"
#include <regex>
#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/compare.hpp"
#include "boost/range/adaptor/transformed.hpp"

namespace is {

auto TransformationPublisher::create_topic(is::Path const& path) -> std::string {
  auto joined = boost::algorithm::join(
      path | boost::adaptors::transformed([](int64_t i) { return std::to_string(i); }), ".");
  return "FrameTransformation." + joined;
}

TransformationPublisher::TransformationPublisher(Channel const& channel, Subscription* subscription,
                                                 std::shared_ptr<opentracing::Tracer> const& tracer,
                                                 DependencyTracker* tracker,
                                                 FrameConversion* conversions)
    : _channel(channel), _tracer(tracer), _tracker(tracker), _conversions(conversions) {
  subscription->subscribe("#.Pose");
}

auto TransformationPublisher::run(Message const& msg) -> bool {
  if (!std::regex_match(msg.topic(), std::regex(".+\\.Pose"))) { return false; }

  auto maybe_ctx = msg.extract_tracing(_tracer);
  auto root = maybe_ctx ? _tracer->StartSpan("UpdateTFs", {opentracing::ChildOf(maybe_ctx->get())})
                        : _tracer->StartSpan("UpdateTFs");

  auto tf = msg.unpack<is::vision::FrameTransformation>();
  if (!tf) {
    is::warn("source=TFPublisher, event=BadSchema");
    return true;
  }

  if (!tf->has_tf()) {
    /* If there is no tf we check if the message comes from a dynamic source. Dynamic sources are
     * usually poses coming from a detection process that can fail. The empty tf indicates that
     * the proccess failed. Therefore all the poses related to that source should be removed
     * otherwise we will compute new tfs using outdated values.
     */
    auto topic = msg.topic();
    auto match = std::smatch{};
    if (std::regex_match(topic, match, std::regex("ArUco.(\\d+).Pose"))) {
      auto id = std::stoi(match[1]);

      std::vector<is::Edge> edges_to_remove;
      edges_to_remove.reserve(16);
      for (auto key_val : _conversions->transformations()) {
        auto edge = key_val.first;
        // if edge goes from the camera id to any id on the dynamic range, select it for later
        // removal
        if (edge.from == id && (edge.to >= 100 && edge.to <= 150)) {
          edges_to_remove.push_back(edge);
        }
      }

      for (auto edge : edges_to_remove) {
        is::info("source=TFPublisher, event=RemoveTF, key={}", edge);
        _conversions->remove_transformation(edge);
      }
    } else {
      is::warn("source=TFPublisher, event=EmptyTF, topic={}", topic);
    }

    return true;
  }

  // Recompute transformation using the graphs calculated earlier
  _tracker->update(*tf, [&](is::Path const& path, is::common::Tensor const& new_tf) {
    // auto span = _tracer->StartSpan("NewTF", {opentracing::ChildOf(&root->context())});
    auto topic = create_topic(path);
    // reply.inject_tracing(_tracer, span->context());
    _tensors[topic] = new_tf;
    // is::info("source=TFPublisher, event=UpdateTF, topic={}, react_to={}", topic, msg.topic());
  });

  auto now = std::chrono::system_clock::now();
  if (now >= _publish_deadline) {
    for (auto&& key_val : _tensors) {
      auto topic = key_val.first;
      auto reply = is::Message{key_val.second};
      _channel.publish(topic, reply);
    }
    _tensors.clear();
    _publish_deadline = now + throttle_interval;
  }

  return true;
}

}  // namespace is
