#pragma once
#include "unidirected_graph.hpp"

template<typename Policy, typename T>
UnidirectedGraph<T> graph_visit(const UnidirectedGraph<T>& grafo, const T& nodo_sorgente, Policy& policy) {
    UnidirectedGraph<T> albero_risultante;
    std::set<T> reached;

    policy.put(nodo_sorgente);
    reached.insert(nodo_sorgente);

    while(!policy.empty()) {
        T u = policy.get();
        for (const auto& v : grafo.neighbours(u)){
            if(reached.contains(v)) continue;
            reached.insert(v);
            policy.put(v);
            // copio il componente del ramo originale u-v, altrimenti l'albero
            // perde resistenze/generatori e i cicli stampano archi vuoti
            for (const auto& e : grafo.incident_edges(u)) {
                T other = (e.from() == u) ? e.to() : e.from();
                if (other == v) {
                    albero_risultante.add_edge(e);
                    break;
                }
            }
        }
    }

    return albero_risultante;
}
