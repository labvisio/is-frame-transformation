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
                                                 DependencyTracker* tracker)
    : _channel(channel), _tracer(tracer), _tracker(tracker) {
  subscription->subscribe("#.Pose");
}

auto TransformationPublisher::run(Message const& msg) -> bool {
  if (!std::regex_match(msg.topic(), std::regex(".+\\.Pose"))) { return false; }

  auto maybe_ctx = msg.extract_tracing(_tracer);
  auto root = maybe_ctx ? _tracer->StartSpan("UpdateTFs", {opentracing::ChildOf(maybe_ctx->get())})
                        : _tracer->StartSpan("UpdateTFs");

  auto tf = msg.unpack<is::vision::FrameTransformation>();
  if (tf) {
    // Recompute transformation using the graphs calculated earlier
    // TODO: Recompute the path to search for a possible new better path
    _tracker->update(*tf, [&](is::Path const& path, is::common::Tensor const& new_tf) {
      auto span = _tracer->StartSpan("NewTF", {opentracing::ChildOf(&root->context())});
      auto topic = create_topic(path);
      auto reply = is::Message{new_tf};
      reply.inject_tracing(_tracer, span->context());
      is::info("[TransformationPublisher] topic='{}'", topic);
      _channel.publish(topic, reply);
    });
  } else {
    is::warn("[TransformationPublisher] error='Wrong Schema'");
  }

  return true;
}

}  // namespace is