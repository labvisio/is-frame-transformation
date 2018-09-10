#pragma once

#include <string>
#include <vector>
#include "boost/functional/hash.hpp"
#include <iostream>

namespace is {

struct Edge {
  int64_t from;
  int64_t to;

  Edge(std::string const& from, std::string const& to)
      : from(std::stoll(from)), to(std::stoll(to)) {}
  Edge(int64_t from, int64_t to) : from(from), to(to) {}
  Edge(Edge const&) = default;
  Edge(Edge&&) = default;

  auto operator==(Edge const& other) const -> bool { return from == other.from && to == other.to; }
};

auto inverted(Edge const&) -> Edge;
auto sorted(Edge const&) -> Edge;

auto hash_value(Edge const&) -> std::size_t;
using EdgeHash = boost::hash<Edge>;

using Path = std::vector<int64_t>;
using PathHash = boost::hash<Path>;

std::ostream& operator<<(std::ostream& os, Edge const&);

}  // namespace is

namespace std {
std::ostream& operator<<(std::ostream& os, std::vector<int64_t> const&);
}