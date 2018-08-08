#include "frame_conversion.hpp"
#include "is/msgs/cv.hpp"

namespace is {

template <typename I, typename P>
I adjacent_for_each(I first, I last, P&& predicate) {
  if (first == last) return first;
  auto second = first;
  ++second;
  while (second != last) { predicate(*first++, *second++); }
  return second;
}

template <typename InputIt, typename OutputIt, typename P>
InputIt adjacent_transform_if(InputIt s_first, InputIt s_last, OutputIt d_first, P&& predicate) {
  if (s_first == s_last) return s_first;
  auto s_second = s_first;
  ++s_second;
  while (s_second != s_last) {
    auto result = predicate(*s_first, *s_second);
    *d_first++ = result;
    if (!result) break;
    ++s_first;
    ++s_second;
  }
  return s_second;
}

auto FrameConversion::has_vertex(int64_t id) const -> bool {
  return vertices.find(id) != vertices.end();
}

auto FrameConversion::get_vertex(int64_t id) const -> Vertex {
  return vertices.find(id)->second;
}

auto FrameConversion::add_vertex(int64_t id) -> Vertex {
  if (has_vertex(id)) return get_vertex(id);
  auto vertex = boost::add_vertex(id, graph);
  vertices[id] = vertex;
  return vertex;
}

void FrameConversion::remove_vertex(int64_t) {
  assert(false && "Not implemented");
}

void FrameConversion::add_edge(Edge const& edge, float weight) {
  auto sorted_edge = sorted(edge);
  boost::add_edge(add_vertex(sorted_edge.from), add_vertex(sorted_edge.to), weight, graph);
}

void FrameConversion::remove_edge(Edge const& edge) {
  auto sorted_edge = sorted(edge);
  assert(has_vertex(sorted_edge.from) && has_vertex(sorted_edge.to) &&
         "Trying to remove an edge from vertices that do not exist");
  boost::remove_edge(get_vertex(sorted_edge.from), get_vertex(sorted_edge.to), graph);
}

void FrameConversion::update_transformation(vision::FrameTransformation const& transformation) {
  update_transformation(Edge{transformation.from(), transformation.to()}, transformation.tf());
}

void FrameConversion::update_transformation(Edge const& edge, common::Tensor const& tensor) {
  // TODO: check if matrix is invertible before inserting
  auto matrix = is::to_mat(tensor);
  matrix.convertTo(matrix, CV_64F);

  auto not_found = tensors.find(edge) == tensors.end();
  if (not_found) add_edge(edge);

  tensors[edge] = is::to_tensor(matrix);
  tensors[inverted(edge)] = is::to_tensor(matrix.inv());
}

void FrameConversion::remove_transformation(vision::FrameTransformation const& transformation) {
  remove_transformation(Edge{transformation.from(), transformation.to()});
}

void FrameConversion::remove_transformation(Edge const& edge) {
  auto removed = tensors.erase(edge);
  if (!removed) return;
  tensors.erase(inverted(edge));
  remove_edge(edge);
  // TODO: remove_vertex when there is no edges to it
}

auto FrameConversion::find_path(Edge const& edge) const -> nonstd::expected<Path, std::string> {
  if (!has_vertex(edge.from)) {
    return nonstd::make_unexpected(fmt::format("Invalid frame \"{}\"", edge.from));
  }
  if (!has_vertex(edge.to)) {
    return nonstd::make_unexpected(fmt::format("Invalid frame \"{}\"", edge.to));
  }

  auto from = get_vertex(edge.from);
  auto to = get_vertex(edge.to);

  std::vector<Vertex> predecessors(boost::num_vertices(graph));
  boost::dijkstra_shortest_paths(graph, to, boost::predecessor_map(&predecessors[0]));

  auto path = Path{};
  path.reserve(4);
  while (from != to) {
    auto next = predecessors[from];
    if (next == from)
      return nonstd::make_unexpected(
          fmt::format("Frames \"{}\" and \"{}\" are not connected", edge.from, edge.to));
    path.push_back(graph[from]);
    from = next;
  }
  path.push_back(graph[from]);
  return path;
}

auto FrameConversion::find_path(Path const& p) const -> nonstd::expected<Path, std::string> {
  assert(p.size() >= 2 && "Path size must be atleast 2");
  auto subpaths = std::vector<nonstd::expected<Path, std::string>>{};

  auto find_each_subpath = [this](int64_t from, int64_t to) { return find_path(Edge{from, to}); };
  auto ok = adjacent_transform_if(p.begin(), p.end(), std::back_inserter(subpaths),
                                  find_each_subpath) == p.end();

  if (!ok) return subpaths.back();

  auto concatenated = Path{};
  auto overall_size =
      std::accumulate(subpaths.begin(), subpaths.end(), 0,
                      [](int total, nonstd::expected<Path, std::string> const& path) {
                        return total + path->size();
                      });
  concatenated.reserve(overall_size);

  auto first = subpaths.begin();
  auto last = subpaths.end();
  concatenated.insert(concatenated.end(), (*first)->begin(), (*first)->end());
  ++first;

  while (first != last) {
    concatenated.insert(concatenated.end(), (*first)->begin() + 1, (*first)->end());
    ++first;
  }
  return concatenated;
}

auto FrameConversion::compose_path(Path const& path) const -> common::Tensor {
  assert(path.size() >= 2 && "Path size must be atleast 2");

  auto tf = cv::Mat::eye(4, 4, CV_64F);

  adjacent_for_each(path.begin() + 1, path.end(), [&](int64_t from, int64_t to) {
    auto tensor = tensors.find(Edge{from, to});
    assert(tensor != tensors.end() && "Edge exists but tensor not found");
    tf = is::to_mat(tensor->second) * tf;
  });

  return is::to_tensor(tf);
}

}  // namespace is