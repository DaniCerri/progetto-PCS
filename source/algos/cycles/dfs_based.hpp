#pragma once
#include "../../unidirected_graph/unidirected_graph.hpp"
#include "../visit/graph_visit.hpp"
#include "../visit/stack.hpp"
#include <vector>
#include <list>
#include <set>

// NB: i nodi si contano a partire da 1, non da 0, quindi il conto va da 1 a N nodi

template<typename T>
bool find_path(const UnidirectedGraph<T>& graph, const T& start, const T& end, std::list<T>& path, std::vector<bool>& visited) {
    path.push_back(start);
    visited[start] = true;
    if (start == end) return true;
    
    for(const auto& neighbor : graph.neighbours(start)) {
        if (!visited[neighbor]) {
            if(find_path(graph, neighbor, end, path, visited)) return true;
        }
    }
    path.pop_back();
    return false;
}    


template<typename T>
void find_essential_cycles_dfs(UnidirectedGraph<T>& graph, std::vector<std::list<T>>& essential_cycles) {
    Stack<T> stack;
    UnidirectedGraph<T> support_tree = graph_visit(graph, *graph.all_nodes().begin(), stack);
    UnidirectedGraph<T> co_tree = graph - support_tree;

    for(const auto& edge : co_tree.all_edges()) {
        std::list<T> path;
        // Inizializza a dimensione (N + 1) per gestire gli indici da 1 a N
        std::vector<bool> visited(graph.all_nodes().size() + 1, false);
        
        // Se troviamo il percorso nell'albero di supporto
        if (find_path(support_tree, edge.from(), edge.to(), path, visited)) {
            // Chiudi il ciclo aggiungendo il nodo di partenza alla fine
            path.push_back(edge.from()); 
            
            // Salva il ciclo fondamentale nella collezione
            essential_cycles.push_back(path); 
        }
    }
};
