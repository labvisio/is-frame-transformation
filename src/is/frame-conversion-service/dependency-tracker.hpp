#pragma once

#include <is/msgs/camera.pb.h>
#include <is/wire/core/logger.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "frame-conversion/frame-conversion.hpp"

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

  void add_dependency(Path const& path, Path const& route);

 public:
  DependencyTracker(FrameConversion* conversions);

  auto update_dependency(Path const&) -> boost::optional<vision::FrameTransformation>;
  void remove_dependency(Path const&);
  void invalidate_edge(Edge const&);

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
    // Copy the paths since an update can modify reverse_dependencies and thus invalidate our
    // iterator
    std::vector<Path> paths(reverse_it->second.begin(), reverse_it->second.end());
    // Update each dependent path
    for (auto&& path : paths) {
      auto maybe_transformation = update_dependency(path);
      if (maybe_transformation) { on_update(path, *maybe_transformation); }
    }
  }

  check_unresolved_dependencies(on_update);
}

template <typename F>
void DependencyTracker::check_unresolved_dependencies(F const& on_update) {
  std::vector<Path> paths(unresolved_dependencies.begin(), unresolved_dependencies.end());

  for (auto&& path : paths) {
    auto maybe_transformation = update_dependency(path);
    if (maybe_transformation) {
      info("event=Dependency.Resolved key={}", path);
      on_update(path, *maybe_transformation);
      unresolved_dependencies.erase(path);
    }
  }
}

}  // namespace is