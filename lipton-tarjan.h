#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <vector>

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS>                                               Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor                                                                     VertexDescriptor;
typedef boost::graph_traits<Graph>::edge_descriptor                                                                       EdgeDescriptor;
typedef std::vector<std::vector<EdgeDescriptor>>                                                                          EmbeddingStorage;
typedef boost::iterator_property_map<EmbeddingStorage::iterator, boost::property_map<Graph, boost::vertex_index_t>::type> Embedding; 

struct Partition
{
        std::vector<VertexDescriptor> a, b, c;
}; 

Partition lipton_tarjan(Graph const& g);
