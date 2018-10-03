#include "dependency-tracker.hpp"

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

auto DependencyTracker::update_dependency(Path const& path)
    -> boost::optional<vision::FrameTransformation> {
  auto had_route = direct_dependencies.find(path) != direct_dependencies.end();
  auto route = conversions->find_path(path);

  auto maybe_transformation = boost::optional<vision::FrameTransformation>{};

  if (!route) {
    if (had_route) {
      info("event=Dependency.BecameUnreachable path={}", path);
      remove_dependency(path);
    }
    auto it_and_ok = unresolved_dependencies.insert(path);
    if (it_and_ok.second) { info("event=Dependency.AddUnresolved path={}", path); }
  } else {
    if (had_route) {
      auto old_route = direct_dependencies.find(path)->second;
      if (old_route != route) {
        info("event=Dependency.NewRoute path={} route={}", path, *route);
        // New route is different, remove the old one and add the new one
        remove_dependency(path);
        add_dependency(path, *route);
      }
    } else {
      add_dependency(path, *route);
    }

    vision::FrameTransformation transformation;
    transformation.set_from(path.front());
    transformation.set_to(path.back());
    *(transformation.mutable_tf()) = conversions->compose_path(*route);
    maybe_transformation = transformation;
  }
  return maybe_transformation;
}

void DependencyTracker::add_dependency(Path const& path, Path const& route) {
  // info("event=Dependency.AddDirect key={} value={}", path, route);
  direct_dependencies[path] = route;
  auto insert_each_edge = [&](int64_t from, int64_t to) {
    auto edge = sorted(Edge{from, to});
    // info("event=Dependency.AddReverse key={} value={}", edge, path);
    reverse_dependencies[edge].insert(path);
  };
  adjacent_for_each(route.begin(), route.end(), insert_each_edge);
}

void DependencyTracker::remove_dependency(Path const& path) {
  auto direct_it = direct_dependencies.find(path);
  if (direct_it != direct_dependencies.end()) {
    auto remove_each_edge = [&](int64_t from, int64_t to) {
      auto edge = sorted(Edge{from, to});
      auto reverse_it = reverse_dependencies.find(edge);
      if (reverse_it == reverse_dependencies.end()) { critical("?"); }
      reverse_it->second.erase(path);
      // info("event=Dependency.DelReverse key={} value={}", edge, path);
    };

    // Remove each edge of the reverse dependencies
    auto reverse_keys = direct_it->second;
    adjacent_for_each(reverse_keys.begin(), reverse_keys.end(), remove_each_edge);

    // Remove the direct dependency
    direct_dependencies.erase(direct_it);
    // info("event=Dependency.DelDirect key={}", path);
  } else {
    // Remove from the unresolved dependencies
    unresolved_dependencies.erase(path);
    // info("event=Dependency.DelUnresolved key={}", path);
  }
}

}  // namespace is