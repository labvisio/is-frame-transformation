#include "dependency_tracker.hpp"

namespace is {

DependencyTracker::DependencyTracker(FrameConversion* c) : conversions(c) {}

auto DependencyTracker::sorted_pair(FrameIds const& ids) const -> FrameIds {
  if (ids.first <= ids.second) return ids;
  return std::make_pair(ids.second, ids.first);
};

auto DependencyTracker::add_dependency(FrameIds const& composed_id)
    -> nonstd::expected<ComposedTransformation, std::string> {
  auto composed_tf = conversions->find(composed_id.first, composed_id.second);

  if (composed_tf) {
    add_dependency(composed_tf->ids);
  } else {
    unresolved_dependencies[composed_id.first].insert(composed_id);
    unresolved_dependencies[composed_id.second].insert(composed_id);
    info("[DependencyTracker] Can't resolve {}<->{}", composed_id.first, composed_id.second);
  }
  return composed_tf;
}

void DependencyTracker::add_dependency(std::vector<FrameIds> const& ids) {
  auto composed_id = FrameIds{ids.front().first, ids.back().second};
  direct_dependencies[composed_id] = ids;

  for (auto&& id : ids) {
    auto key = sorted_pair(id);
    reverse_dependencies[key].insert(composed_id);
    info("[DependencyTracker] [+] {}<->{}: {}->{}", key.first, key.second, composed_id.first,
         composed_id.second);
  }
}

void DependencyTracker::remove_dependency(FrameIds const& ids) {
  auto it = direct_dependencies.find(ids);
  if (it != direct_dependencies.end()) {
    for (auto&& id : it->second) {
      auto key = sorted_pair(id);
      reverse_dependencies[key].erase(ids);
      info("[DependencyTracker] [-] {}<->{}: {}->{}", key.first, key.second, ids.first, ids.second);
    }
    direct_dependencies.erase(it);
  } else {
    warn("[DependencyTracker] Not removing {}->{} since it wasn't found", ids.first, ids.second);
  }
}

auto DependencyTracker::find_dependencies(FrameIds const& ids) const
    -> std::vector<std::vector<FrameIds>> {
  std::vector<std::vector<FrameIds>> dependencies;

  auto key = sorted_pair(ids);
  auto reverse_it = reverse_dependencies.find(key);
  if (reverse_it != reverse_dependencies.end()) {
    for (auto ids : reverse_it->second) {
      auto direct_it = direct_dependencies.find(ids);
      if (direct_it == direct_dependencies.end()) {
        error("[DependencyTracker] Impossible state reached");
        break;
      }
      dependencies.push_back(direct_it->second);
    }
  }

  return dependencies;
}

}  // namespace is