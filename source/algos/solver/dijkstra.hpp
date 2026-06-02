# pragma once
# include <map>
# include <set>
# include <limits>
# include <utility>
# include "../../unidirected_graph/unidirected_graph.hpp"

template <typename T>
std::pair<std::map<T, int>, std::map<T, T>> dijkstra(const UnidirectedGraph<T>& G, T node) {
    
    std::map<T, int> dist;
    std::map<T, T> pred;

    for (T nodo : G.all_nodes()) {
        dist[nodo] = std::numeric_limits<int>::max();
    }
    pred[node] = node;
    dist[node] = 0;

    std::set<std::pair<int, T>> PQ; 

    for (T nodo : G.all_nodes()) {
        PQ.insert({dist[nodo], nodo});
    }
    while (!PQ.empty()) {
        T u = PQ.begin()->second; 
        int p = PQ.begin()->first;
        PQ.erase(PQ.begin());
        if (p == std::numeric_limits<int>::max()) break;

        for (T w : G.neighbours(u)) {
            int weight = 1;
            if (dist[w] > dist[u] + weight) {
                PQ.erase({dist[w], w}); 
                dist[w] = dist[u] + weight;
                pred[w] = u;
                PQ.insert({dist[w], w});
            }
        }
    }
    return {dist, pred}; 
}




// Scriviamo una funzione che costruisce un vettore di incidenza per ogni ciclo trovato con dijkstra tra nodo 1 a nodo2, in modo da poter applicare l'algoritmo di De Pina per trovare i cicli essenziali in un grafo non orientato.
template <typename T>
std::vector<bool> find_incidence_vector(const UnidirectedGraph<T>& graph, const std::map<std::pair<T,bool>, std::pair<T,bool>>& pred, std::pair<T,bool> start_node, std::pair<T,bool> end_node) {
    // Creiamo un vettore di incidenza inizializzato a false
    auto s = graph.all_edges();
    std::vector<UnidirectedEdge<T>> edge_list(s.begin(), s.end());
    std::vector<bool> incidence_vector(graph.all_edges().size(), false);

    auto current = end_node;

    while (current != start_node) {
        if (pred.find(current) == pred.end()) break; // Se non troviamo un predecessore, usciamo dal ciclo
        std::pair<T,bool> parent = pred.at(current);

        // Troviamo l'indice dell'arco (parent, node) o (node, parent) nel grafo originale
        for (int i = 0; i < edge_list.size(); ++i) {
            auto edge = edge_list[i];
            if ((edge.from() == current.first && edge.to() == parent.first) || (edge.from() == parent.first && edge.to() == current.first)) {
                incidence_vector[i] = !incidence_vector[i]; // Incrementiamo mod 2 l'incidenza dell'arco trovato
                break;
            }
        }
        current = parent; // Saltiamo al nodo padre
    }
    return incidence_vector;
}
