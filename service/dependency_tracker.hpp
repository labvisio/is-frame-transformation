#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "frame_conversion/frame_conversion.hpp"
#include "is/wire/core/logger.hpp"

namespace is {

class DependencyTracker {
  /* Mapping between the user desired transformation and the full path, e.g:
    Path{1 -> 2}: Path{1 -> 10 -> 3 -> 2} */
  std::unordered_map<Path, Path, PathHash> direct_dependencies;

  /* Maps which Paths depend on the given Edge, e.g (for the example before this would be):
    Edge{1 -> 10}: [ Path{1 -> 2} ]
    Edge{2 -> 3}:  [ Path{1 -> 2} ]
    Edge{3 -> 10}: [ Path{1 -> 2} ]
  */
  std::unordered_map<Edge, std::unordered_set<Path, PathHash>, EdgeHash> reverse_dependencies;

  /* Keep track of paths that we were unable to solve for each node. e.g
  (if user requests Path{1->2} and we are unable to solve it)
    1: [ Path{1->2} ]
    2: [ Path{1->2} ]
  */
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
  // Find all paths that depend on this edge
  auto reverse_it = reverse_dependencies.find(sorted(edge));
  if (reverse_it != reverse_dependencies.end()) {
    // Update each dependent path
    for (auto path : reverse_it->second) {
      auto direct_it = direct_dependencies.find(path);
      assert(direct_it != direct_dependencies.end() &&
             "Invalid state reverse dependency point to non existing direct dependency");
      on_update(direct_it->first, conversions->compose_path(direct_it->second));
    }
  }

  // Check both nodes for unresolved dependencies
  check_unresolved_dependencies(tf.from(), on_update);
  check_unresolved_dependencies(tf.to(), on_update);
}

template <typename F>
void DependencyTracker::check_unresolved_dependencies(int64_t id, F const& on_update) {
  // TODO: This procedure only works for cases where a vertex does not exist. It will not be enough
  // for cases where there is no path and an update causes it to exist. For this last case, an
  // update will be triggered only when one of the outer vertices change.

  auto unresolved_it = unresolved_dependencies.find(id);
  // If this node has unresolved dependencies.
  if (unresolved_it != unresolved_dependencies.end()) {
    auto& unresolved = unresolved_it->second;

    // For each unresolved dependency/path.
    for (auto path = unresolved.begin(); path != unresolved.end();) {
      // Try to find path
      auto solution = conversions->find_path(*path);
      if (solution) {
        info("[DependencyTracker][Resolved] {}", *path);
        // TODO: Avoid recomputing the solution
        update_dependency(*path);
        on_update(*path, conversions->compose_path(*solution));
        path = unresolved.erase(path);
      } else {
        ++path;
      }
    }
  }
}

}  // namespace is