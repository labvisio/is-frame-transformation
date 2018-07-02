#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "frame_conversion/frame_conversion.hpp"
#include "is/wire/core/logger.hpp"

namespace is {

class DependencyTracker {
  std::unordered_map<FrameIds, std::vector<FrameIds>, FrameIdsHash> direct_dependencies;
  std::unordered_map<FrameIds, std::unordered_set<FrameIds, FrameIdsHash>, FrameIdsHash>
      reverse_dependencies;
  std::unordered_map<int64_t, std::unordered_set<FrameIds, FrameIdsHash>> unresolved_dependencies;
  FrameConversion* conversions;

  auto sorted_pair(FrameIds const& ids) const -> FrameIds;
  auto find_dependencies(FrameIds const& ids) const -> std::vector<std::vector<FrameIds>>;
  void add_dependency(std::vector<FrameIds> const& ids);

  template <typename F>
  void check_unresolved_dependencies(int64_t id, F const& on_update);

 public:
  DependencyTracker(FrameConversion*);

  auto add_dependency(FrameIds const& ids) -> nonstd::expected<ComposedTransformation, std::string>;
  void remove_dependency(FrameIds const& ids);

  template <typename F>
  void update(vision::FrameTransformation const& tf, F const& on_update);
};

template <typename F>
void DependencyTracker::update(vision::FrameTransformation const& tf, F const& on_update) {
  conversions->add(tf);
  auto dependencies = find_dependencies(FrameIds{tf.from(), tf.to()});
  for (auto&& dependency : dependencies) {
    vision::FrameTransformation new_tf;
    new_tf.set_from(dependency.front().first);
    new_tf.set_to(dependency.back().second);
    *new_tf.mutable_tf() = conversions->compose(dependency);

    on_update(new_tf);
  }
  check_unresolved_dependencies(tf.from(), on_update);
  check_unresolved_dependencies(tf.to(), on_update);
}

template <typename F>
void DependencyTracker::check_unresolved_dependencies(int64_t id, F const& on_update) {
  // TODO: This procedure only works for cases where a vertex does not exist. It will not be enough
  // for cases where there is no path and an update causes it to exist. For this last case, an
  // update will be triggered only when one of the outer vertices change.
  auto unresolved_it = unresolved_dependencies.find(id);
  if (unresolved_it != unresolved_dependencies.end()) {
    auto& ids = unresolved_it->second;

    for (auto it = ids.begin(); it != ids.end();) {
      auto composed_tf = conversions->find(it->first, it->second);
      if (composed_tf) {
        info("[DependencyTracker] Resolved {}<->{}", it->first, it->second);
        add_dependency(composed_tf->ids);

        vision::FrameTransformation new_tf;
        new_tf.set_from(it->first);
        new_tf.set_to(it->second);
        *new_tf.mutable_tf() = composed_tf->tf;
        on_update(new_tf);

        it = ids.erase(it);
      } else {
        ++it;
      }
    }
  }
}

}  // namespace is