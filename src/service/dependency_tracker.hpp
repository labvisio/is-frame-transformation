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

  // Keep track of paths that we were unable to solve.
  std::unordered_set<Path, PathHash> unresolved_dependencies;

  // Transformation graph solver.
  FrameConversion* conversions;

  template <typename F>
  void check_unresolved_dependencies(F const& on_update);

 public:
  DependencyTracker(FrameConversion* conversions);

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
      auto maybe_tensor = update_dependency(path);
      // if no route was found this iterator was invalidated leave for now. 
      if (!maybe_tensor) break;
      on_update(path, *maybe_tensor); 
    }
  }

  check_unresolved_dependencies(on_update);
}

template <typename F>
void DependencyTracker::check_unresolved_dependencies(F const& on_update) {
  for (auto unresolved_it = unresolved_dependencies.begin();
       unresolved_it != unresolved_dependencies.end();) {
    auto solution = conversions->find_path(*unresolved_it);
    if (solution) {
      info("source=DependencyTracker, event=Resolved, key={}", *unresolved_it);
      // TODO: Avoid recomputing the solution
      update_dependency(*unresolved_it);
      on_update(*unresolved_it, conversions->compose_path(*solution));
      unresolved_it = unresolved_dependencies.erase(unresolved_it);
    } else {
      ++unresolved_it;
    }
  }
}

}  // namespace is