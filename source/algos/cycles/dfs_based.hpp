#pragma once
#include "../../unidirected_graph/unidirected_graph.hpp"
#include "../visit/graph_visit.hpp"
#include "../visit/stack.hpp"
#include <vector>
#include <list>
#include <set>

// visited e' un set indicizzato per valore: etichette di nodo non contigue
// (es. {1,2,5,10,20}) non causano accessi fuori dai limiti.
template<typename T>
bool find_path(const UnidirectedGraph<T>& graph, const T& u, const T& v,
               std::vector<UnidirectedEdge<T>>& path, std::set<T>& visited) {

    visited.insert(u);

    if (u == v) return true;

    for (const auto& edge : graph.incident_edges(u)) {
        T neighbor = (edge.from() == u) ? edge.to() : edge.from();

        if (!visited.contains(neighbor)) {
            path.push_back(edge);

            if (find_path(graph, neighbor, v, path, visited)) {
                return true;
            }

            path.pop_back();   // backtracking
        }
    }

    return false;
}

template<typename T>
void find_essential_cycles_dfs(UnidirectedGraph<T>& graph, std::vector<std::vector<T>>& essential_cycles) {
    Stack<T> stack;
    UnidirectedGraph<T> support_tree = graph_visit(graph, *graph.all_nodes().begin(), stack);
    UnidirectedGraph<T> co_tree = graph - support_tree;

    const size_t num_nodes = graph.all_nodes().size();

    std::vector<UnidirectedEdge<T>> path_buffer;
    path_buffer.reserve(num_nodes);
    std::set<T> visited;

    for (const auto& edge : co_tree.all_edges()) {
        path_buffer.clear();
        visited.clear();

        if (find_path(support_tree, edge.from(), edge.to(), path_buffer, visited)) {
            // ricostruisco la sequenza di nodi camminando il cammino from->to.
            // La corda del co-albero chiude la maglia in wrap-around: non la aggiungo.
            std::vector<T> nodes;
            nodes.reserve(path_buffer.size() + 1);
            T cur = edge.from();
            nodes.push_back(cur);
            for (const auto& e : path_buffer) {
                cur = (e.from() == cur) ? e.to() : e.from();
                nodes.push_back(cur);
            }

            essential_cycles.push_back(nodes);
        }
    }
}
