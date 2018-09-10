#include "edge.hpp"

namespace is {

auto hash_value(Edge const& edge) -> std::size_t {
  std::size_t seed = 0;
  boost::hash_combine(seed, boost::hash_value(edge.from));
  boost::hash_combine(seed, boost::hash_value(edge.to));
  return seed;
}

auto inverted(Edge const& edge) -> Edge {
  return Edge{edge.to, edge.from};
}

auto sorted(Edge const& edge) -> Edge {
  if (edge.from <= edge.to) return edge;
  return Edge{edge.to, edge.from};
}

std::ostream& operator<<(std::ostream& os, Edge const& e) {
  os << '\"' << e.from << " -> " << e.to << '\"';
}

}  // namespace is

namespace std {
std::ostream& operator<<(std::ostream& os, std::vector<int64_t> const& p) {
  os << '\"';
  if (!p.empty()) {
    auto first = p.begin();
    auto last = p.end();
    --last;
    while (first != last) {
      os << *first << " -> ";
      ++first;
    }
    os << *first;
  }
  os << '\"';
}
}