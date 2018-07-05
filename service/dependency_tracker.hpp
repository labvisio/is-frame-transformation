#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "frame_conversion/frame_conversion.hpp"
#include "is/wire/core/logger.hpp"

namespace is {

class DependencyTracker {
  std::unordered_map<Path, Path, PathHash> direct_dependencies;
  std::unordered_map<Edge, std::unordered_set<Path, PathHash>, EdgeHash> reverse_dependencies;
  std::unordered_map<int64_t, std::unordered_set<Path, PathHash>> unresolved_dependencies;
  FrameConversion* conversions;

  template <typename F>
  void check_unresolved_dependencies(int64_t id, F const& on_update);

 public:
  DependencyTracker(FrameConversion*);

  auto update_dependency(Path const&) -> boost::optional<common::Tensor>;
  void remove_dependency(Path const&);

  template <typename F>
  void update(vision::FrameTransformation const& tf, F const& on_update);
};

template <typename F>
void DependencyTracker::update(vision::FrameTransformation const& tf, F const& on_update) {
  conversions->update_transformation(tf);

  auto edge = Edge{tf.from(), tf.to()};
  auto reverse_it = reverse_dependencies.find(sorted(edge));
  if (reverse_it != reverse_dependencies.end()) {
    for (auto path : reverse_it->second) {
      auto direct_it = direct_dependencies.find(path);
      assert(direct_it != direct_dependencies.end() &&
             "Invalid state reverse dependency point to non existing direct dependency");

      on_update(direct_it->first, conversions->compose_path(direct_it->second));
    }
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
    auto& unresolved = unresolved_it->second;

    for (auto it = unresolved.begin(); it != unresolved.end();) {
      auto path = conversions->find_path(*it);
      if (path) {
        info("[DependencyTracker][Resolved] {}", *it);
        // TODO: Avoid recomputation of path
        update_dependency(*it);
        on_update(*it, conversions->compose_path(*path));
        it = unresolved.erase(it);
      } else {
        ++it;
      }
    }
  }
}

}  // namespace is