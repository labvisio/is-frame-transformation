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
  auto route = conversions->find_path(path);

  if (!route) {
    for (auto&& vertex : path) {
      unresolved_dependencies[vertex].insert(path);
      info("[DependencyTracker][+][Unresolved] key={} value={}", vertex, path);
    }
    return boost::none;
  }

  direct_dependencies[path] = *route;
  info("[DependencyTracker][+][Direct] key={} value={}", path, *route);

  auto insert_each_edge = [&](int64_t from, int64_t to) {
    auto edge = sorted(Edge{from, to});
    reverse_dependencies[edge].insert(path);
    info("[DependencyTracker][+][Reverse] key={} value={}", edge, path);
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
      info("[DependencyTracker][-][Reverse] key={} value={}", edge, path);
    };
    adjacent_for_each(direct_it->second.begin(), direct_it->second.end(), remove_each_edge);

    direct_dependencies.erase(direct_it);
    info("[DependencyTracker][-][Direct] key={}", path);
  } else {
    for (auto&& vertex : path) {
      auto unresolved_it = unresolved_dependencies.find(vertex);
      if (unresolved_it != unresolved_dependencies.end()) {
        unresolved_it->second.erase(path);
        info("[DependencyTracker][-][Unresolved] key={} value={}", vertex, path);
      }
    }
  }
}

}  // namespace is