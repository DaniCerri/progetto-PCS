#pragma once
#include "../../unidirected_graph/unidirected_graph.hpp"
#include "../visit/graph_visit.hpp"
#include "../visit/stack.hpp"
#include <vector>
#include <list>
#include <set>

// Manteniamo esattamente la firma e la logica dell'Algoritmo 1 del PoliTo.
// visited e' un std::set<T>: i nodi sono indicizzati per valore in un contenitore
// associativo, non per posizione in un vettore. Cosi' etichette di nodo non
// contigue o piu' grandi del numero di nodi (es. {1,2,5,10,20}) non causano
// accessi fuori dai limiti.
template<typename T>
bool find_path(const UnidirectedGraph<T>& graph, const T& u, const T& v,
               std::vector<UnidirectedEdge<T>>& path, std::set<T>& visited) {

    visited.insert(u);

    if (u == v) return true;

    // Sfruttiamo il nuovo metodo incident_edges che interroga solo i rami adiacenti
    for (const auto& edge : graph.incident_edges(u)) {

        // CORREZIONE BUG: Determina chi è il vicino in un grafo non orientato
        T neighbor = (edge.from() == u) ? edge.to() : edge.from();

        if (!visited.contains(neighbor)) {
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
void find_essential_cycles_dfs(UnidirectedGraph<T>& graph, std::vector<std::vector<T>>& essential_cycles) {
    Stack<T> stack;
    UnidirectedGraph<T> support_tree = graph_visit(graph, *graph.all_nodes().begin(), stack);
    UnidirectedGraph<T> co_tree = graph - support_tree;

    const size_t num_nodes = graph.all_nodes().size();

    std::vector<UnidirectedEdge<T>> path_buffer;
    path_buffer.reserve(num_nodes);
    std::set<T> visited;   // nodi visitati, indicizzati per valore (no assunzioni sulle etichette)

    for (const auto& edge : co_tree.all_edges()) {
        path_buffer.clear();
        visited.clear();

        // Cerchiamo il cammino nell'albero di supporto tra i due estremi dell'arco del co-albero
        if (find_path(support_tree, edge.from(), edge.to(), path_buffer, visited)) {

            // path_buffer: archi dell'albero in ordine da edge.from() a edge.to().
            // Ricostruisco la sequenza ordinata di nodi camminando lungo il cammino
            // (archi gia' adiacenti, nessun rischio di salto). La corda del co-albero
            // (edge.to() -> edge.from()) chiude la maglia in wrap-around: NON la
            // aggiungo, il nodo di chiusura e' implicito.
            std::vector<T> nodes;
            nodes.reserve(path_buffer.size() + 1);
            T cur = edge.from();
            nodes.push_back(cur);
            for (const auto& e : path_buffer) {
                cur = (e.from() == cur) ? e.to() : e.from();
                nodes.push_back(cur);   // ...fino a edge.to()
            }

            // Salva il ciclo fondamentale come sequenza di nodi (verso di percorrenza)
            essential_cycles.push_back(nodes);
        }
    }
}