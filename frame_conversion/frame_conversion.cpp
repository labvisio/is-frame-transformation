#include "frame_conversion.hpp"
#include "is/msgs/cv.hpp"

namespace is {

auto FrameConversion::add_vertex(int64_t id) -> FrameConversion::Vertex {
  auto it = vertices.find(id);
  if (it == vertices.end()) {
    auto vertex = boost::add_vertex(id, graph);
    vertices[id] = vertex;
    return vertex;
  }
  return it->second;
}

void FrameConversion::add_edge(int64_t from, int64_t to, float weight) {
  boost::add_edge(add_vertex(from), add_vertex(to), weight, graph);
}

void FrameConversion::add(vision::FrameTransformation const& transformation) {
  add_transformation(transformation.from(), transformation.to(), transformation.tf());
}

void FrameConversion::add_transformation(int64_t from, int64_t to, common::Tensor const& tensor) {
  auto matrix = is::to_mat(tensor);
  matrix.convertTo(matrix, CV_64F);
  tfs[std::make_pair(from, to)] = is::to_tensor(matrix);
  tfs[std::make_pair(to, from)] = is::to_tensor(matrix.inv());
  add_edge(from, to);
}

void FrameConversion::remove_transformation(int64_t from, int64_t to) {
  tfs.erase(std::make_pair(from, to));
  tfs.erase(std::make_pair(to, from));
}

auto FrameConversion::find(int64_t from, int64_t to) const
    -> nonstd::expected<ComposedTransformation, std::string> {
  auto from_vertex = vertices.find(from);
  if (from_vertex == vertices.end()) {
    return nonstd::make_unexpected(fmt::format("Invalid frame \"{}\"", from));
  }

  auto to_vertex = vertices.find(to);
  if (to_vertex == vertices.end()) {
    return nonstd::make_unexpected(fmt::format("Invalid frame \"{}\"", to));
  }

  std::vector<Vertex> predecessors(boost::num_vertices(graph));
  boost::dijkstra_shortest_paths(graph, to_vertex->second,
                                 boost::predecessor_map(&predecessors[0]));

  ComposedTransformation composition;
  auto first = from_vertex->second;
  auto last = to_vertex->second;
  while (first != last) {
    auto second = predecessors[first];
    if (second == first)
      return nonstd::make_unexpected(
          fmt::format("Frames \"{}\" and \"{}\" are not connected", from, to));

    composition.ids.emplace_back(graph[first], graph[second]);
    first = second;
  }

  composition.tf = compose(composition.ids);
  return composition;
}

auto FrameConversion::compose(std::vector<FrameIds> path) const -> common::Tensor {
  auto tf = cv::Mat::eye(4, 4, CV_64F);
  for (auto&& id : path) {
    auto tensor = tfs.find(id);
    assert(tensor != tfs.end());
    tf = is::to_mat(tensor->second) * tf;
  }
  return is::to_tensor(tf);
}

}  // namespace is