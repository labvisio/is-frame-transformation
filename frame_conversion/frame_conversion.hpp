#pragma once

#include <vector>
#include "boost/functional/hash.hpp"
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/dijkstra_shortest_paths.hpp"
#include "boost/graph/graph_traits.hpp"
#include "boost/optional.hpp"
#include "expected.hpp"
#include "is/msgs/camera.pb.h"
#include "is/msgs/common.pb.h"
#include "spdlog/spdlog.h"

namespace is {

using FrameIds = std::pair<int64_t, int64_t>;
using FrameIdsHash = boost::hash<FrameIds>;

struct ComposedTransformation {
  // the result of the composition of transformations
  common::Tensor tf;
  // ids of the frames used to compose this transformation
  std::vector<FrameIds> ids;
};

class FrameConversion {
  using Graph = boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, int64_t,
                                      boost::property<boost::edge_weight_t, float>>;
  using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

  std::unordered_map<FrameIds, common::Tensor, FrameIdsHash> tfs;
  Graph graph;
  std::unordered_map<int64_t, Vertex> vertices;

  auto add_vertex(int64_t id) -> Vertex;
  void add_edge(int64_t from, int64_t to, float weight = 1.0);
  void add_transformation(int64_t from, int64_t to, common::Tensor const&);
  void remove_transformation(int64_t from, int64_t to);

 public:
  FrameConversion() = default;
  FrameConversion(FrameConversion const&) = default;
  FrameConversion(FrameConversion&&) = default;

  void add(vision::FrameTransformation const&);
  auto find(int64_t from, int64_t to) const
      -> nonstd::expected<ComposedTransformation, std::string>;
  auto compose(std::vector<FrameIds> path) const -> common::Tensor;
};

}  // namespace is