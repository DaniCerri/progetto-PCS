#pragma once
#include "../../unidirected_graph/unidirected_graph.hpp"
#include "../visit/graph_visit.hpp"
#include "../visit/stack.hpp"
#include <vector>
#include <list>

// Manteniamo esattamente la firma e la logica dell'Algoritmo 1 del PoliTo
template<typename T>
bool find_path(const UnidirectedGraph<T>& graph, const T& u, const T& v, 
               std::vector<UnidirectedEdge<T>>& path, std::vector<bool>& visited) {
    
    visited[u] = true;
    
    if (u == v) return true;
    
    // Sfruttiamo il nuovo metodo incident_edges che interroga solo i rami adiacenti
    for (const auto& edge : graph.incident_edges(u)) {
        
        // CORREZIONE BUG: Determina chi è il vicino in un grafo non orientato
        T neighbor = (edge.from() == u) ? edge.to() : edge.from();
        
        if (!visited[neighbor]) {
            // Salva l'arco (con le sue resistenze/componenti interni) prima di scendere in profondità
            path.push_back(edge); 
            
            if (find_path(graph, neighbor, v, path, visited)) {
                return true;
            }
            
            // Backtracking: rimuovi l'arco se questo ramo non porta a 'v'
            path.pop_back(); 
        }
    }
    
    return false;
}  

template<typename T>
void find_essential_cycles_dfs(UnidirectedGraph<T>& graph, std::vector<std::vector<UnidirectedEdge<T>>>& essential_cycles) {
    Stack<T> stack;
    UnidirectedGraph<T> support_tree = graph_visit(graph, *graph.all_nodes().begin(), stack);
    UnidirectedGraph<T> co_tree = graph - support_tree;

    const size_t num_nodes = graph.all_nodes().size();
    
    std::vector<UnidirectedEdge<T>> path_buffer;
    path_buffer.reserve(num_nodes); 
    std::vector<bool> visited(num_nodes + 1, false);

    for (const auto& edge : co_tree.all_edges()) {
        path_buffer.clear();
        visited.assign(num_nodes + 1, false); 
        
        // Cerchiamo il cammino nell'albero di supporto tra i due estremi dell'arco del co-albero
        if (find_path(support_tree, edge.from(), edge.to(), path_buffer, visited)) {
            
            // L'arco del co-albero (edge) chiude l'anello.
            // Contiene già i nodi e i relativi componenti interni
            path_buffer.push_back(edge); 
            
            // Salva il ciclo fondamentale composto da archi
            essential_cycles.push_back(path_buffer); 
        }
    }
}