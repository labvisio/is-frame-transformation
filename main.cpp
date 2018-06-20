// https://theboostcpplibraries.com/boost.graph-algorithms

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/transform_value_property_map.hpp>
#include <fstream>
#include <iostream>

int main(int, char* []) {
  class Weight {
    float f;

   public:
    Weight(float v = 0) : f(v){};
    float weight() const { return f; };
  };

  using Graph = boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS,
                                      boost::no_property, Weight>;
  using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

  Graph graph;
  Vertex v0 = boost::add_vertex(graph);
  Vertex v1 = boost::add_vertex(graph);
  Vertex v2 = boost::add_vertex(graph);
  Vertex v100 = boost::add_vertex(graph);

  boost::add_edge(v0, v100, Weight{5.0}, graph);
  boost::add_edge(v0, v1, Weight{1.0}, graph);
  boost::add_edge(v1, v2, Weight{2.0}, graph);
  boost::add_edge(v2, v100, Weight{0.1}, graph);

  std::vector<Vertex> predecessors(boost::num_vertices(graph));

  auto wmap = boost::make_transform_value_property_map([](Weight& w) { return w.weight(); },
                                                       get(boost::edge_bundle, graph));
  boost::dijkstra_shortest_paths(graph, v100,
                                 boost::predecessor_map(&predecessors[0]).weight_map(wmap));

  Vertex v = v0;
  while (v != v100) {
    std::cout << v << " -> ";
    v = predecessors[v];
  }
  std::cout << v << '\n';
}
