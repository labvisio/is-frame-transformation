#include "dependency_tracker.hpp"

namespace is {

template <typename I, typename P>
I adjacent_for_each(I first, I last, P&& predicate) {
  if (first == last) return first;
  auto second = first;
  ++second;
  while (second != last) { predicate(*first++, *second++); }
  return second;
}

DependencyTracker::DependencyTracker(FrameConversion* c) : conversions(c) {}

auto DependencyTracker::update_dependency(Path const& path) -> boost::optional<common::Tensor> {
  auto had_route = direct_dependencies.find(path) != direct_dependencies.end();
  auto route = conversions->find_path(path);

  if (!route) {
    if (had_route) {
      info("source=DependencyTracker, event=Unreachable, key={}", path);
      remove_dependency(path);
    }

    unresolved_dependencies.insert(path);
    info("source=DependencyTracker, event=AddUnresolved, key={}", path);
    return boost::none;
  }

  if (had_route) {
    auto old_route = direct_dependencies.find(path)->second;
    // If the new route is equal to the old we had compute tf and leave
    if (old_route == route) { return conversions->compose_path(*route); }

    // Else we have a new route remove the old one
    info("source=DependencyTracker, event=NewRoute, key={}", path);
    remove_dependency(path);
  }

  // Add the new route to our tracking data structures
  direct_dependencies[path] = *route;
  info("source=DependencyTracker, event=AddDirect, key={} value={}", path, *route);

  auto insert_each_edge = [&](int64_t from, int64_t to) {
    auto edge = sorted(Edge{from, to});
    reverse_dependencies[edge].insert(path);
    info("source=DependencyTracker, event=AddReverse, key={} value={}", edge, path);
  };
  adjacent_for_each(route->begin(), route->end(), insert_each_edge);

  return conversions->compose_path(*route);
}

void DependencyTracker::remove_dependency(Path const& path) {
  auto direct_it = direct_dependencies.find(path);
  if (direct_it != direct_dependencies.end()) {
    auto remove_each_edge = [&](int64_t from, int64_t to) {
      auto edge = sorted(Edge{from, to});
      auto reverse_it = reverse_dependencies.find(edge);
      assert(reverse_it != reverse_dependencies.end());
      reverse_it->second.erase(path);
      info("source=DependencyTracker, event=DelReverse, key={} value={}", edge, path);
    };

    // Remove each edge of the reverse dependencies
    auto reverse_keys = direct_it->second;
    adjacent_for_each(reverse_keys.begin(), reverse_keys.end(), remove_each_edge);

    // Remove the direct dependency
    direct_dependencies.erase(direct_it);
    info("source=DependencyTracker, event=DelDirect, key={}", path);
  } else {
    // Remove from the unresolved dependencies
    unresolved_dependencies.erase(path);
    info("source=DependencyTracker, event=DelUnresolved, key={}", path);
  }
}

}  // namespace is