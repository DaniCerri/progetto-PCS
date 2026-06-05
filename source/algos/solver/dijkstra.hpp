# pragma once
# include <map>
# include <set>
# include <vector>
# include <algorithm>
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




// Estrae la sequenza ORDINATA dei nodi del grafo originale seguendo la catena
// pred del cammino minimo nel grafo sollevato (start = v+, end = v-).
// I nodi sollevati (T,bool) si proiettano sulla prima componente: e' esattamente
// il verso di percorrenza del ciclo minimo. start ed end proiettano sullo stesso
// nodo base v, quindi si rimuove il duplicato di chiusura finale.
// Cosi' De Pina espone i cicli gia' come cammino ordinato, senza riordini a valle.
template <typename T>
std::vector<T> extract_cycle_nodes(
    const std::map<std::pair<T,bool>, std::pair<T,bool>>& pred,
    std::pair<T,bool> start_node,
    std::pair<T,bool> end_node)
{
    std::vector<T> nodes;
    auto current = end_node;
    while (true) {
        nodes.push_back(current.first);
        if (current == start_node) break;
        auto it = pred.find(current);
        if (it == pred.end()) break;   // difensivo: catena interrotta
        current = it->second;
    }
    std::reverse(nodes.begin(), nodes.end());
    // start ed end proiettano sullo stesso nodo base -> tolgo il doppione di chiusura
    if (nodes.size() >= 2 && nodes.front() == nodes.back())
        nodes.pop_back();
    return nodes;
}
