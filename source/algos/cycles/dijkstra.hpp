#pragma once
#include <map>
#include <set>
#include <vector>
#include <limits>
#include <utility>
#include "../../unidirected_graph/unidirected_graph.hpp"

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
