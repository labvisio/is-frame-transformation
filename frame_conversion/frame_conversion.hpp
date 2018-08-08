#pragma once

#include <unordered_map>
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/dijkstra_shortest_paths.hpp"
#include "boost/graph/graph_traits.hpp"
#include "boost/optional.hpp"
#include "edge.hpp"
#include "expected.hpp"
#include "is/msgs/camera.pb.h"
#include "is/msgs/common.pb.h"
#include "spdlog/spdlog.h"

namespace is {

class FrameConversion {
  using Graph = boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, int64_t,
                                      boost::property<boost::edge_weight_t, float>>;
  using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

  std::unordered_map<Edge, common::Tensor, EdgeHash> tensors;
  Graph graph;
  std::unordered_map<int64_t, Vertex> vertices;


  auto has_vertex(int64_t id) const -> bool; 
  auto get_vertex(int64_t id) const -> Vertex;
  auto add_vertex(int64_t id) -> Vertex;
  void remove_vertex(int64_t);

  void add_edge(Edge const&, float weight = 1.0);
  void remove_edge(Edge const&);

  void update_transformation(Edge const&, common::Tensor const&);
  void remove_transformation(Edge const&);

 public:
  FrameConversion() = default;
  FrameConversion(FrameConversion const&) = default;
  FrameConversion(FrameConversion&&) = default;

  void update_transformation(vision::FrameTransformation const&);
  void remove_transformation(vision::FrameTransformation const&);

  // Try to find shortest path that connects the two given vertices
  auto find_path(Edge const&) const -> nonstd::expected<Path, std::string>;
  // Try to find shortest path that connects all the vertices
  auto find_path(Path const&) const -> nonstd::expected<Path, std::string>;
  // Compose all the transformations on the given path resulting in a single transformation
  auto compose_path(Path const&) const -> common::Tensor;
};

}  // namespace is