#pragma once
#include "unidirected_graph.hpp"
#include "graph_visit.hpp"
#include "stack.hpp"
#include <vector>
#include <list>
#include <set>

template<typename T>
void find_essential_cycles_dfs(UnidirectedGraph<T>& graph, std::vector<std::list<T>>& essential_cycles) {
    Stack stack;
    UnidirectedGraph<T> support_tree = graph_visit(graph, 1, stack);
    UnidirectedGraph<T> back_edges = graph - support_tree;

    for(const auto& edge : back_edges.all_edges()) {
        
    }
};
