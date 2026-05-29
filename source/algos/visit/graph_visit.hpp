#pragma once
#include "unidirected_graph.hpp"

template<typename Policy, typename T>
UnidirectedGraph<T> graph_visit(const UnidirectedGraph<T>& grafo, const T& nodo_sorgente, Policy& policy) {
    UnidirectedGraph<T> albero_risultante;  // Grafo in cui metteremo l'albero di visita
    std::set<T> reached; //Insieme dei nodi raggiunti

    policy.put(nodo_sorgente); //Inseriamo il nodo sorgente
    reached.insert(nodo_sorgente); //Aggiungiamo il nodo al set dei nodi visitati
    
    while(!policy.empty()) {
        T u = policy.get(); //Prendiamo il nodo
        for (const auto& v : grafo.neighbours(u)){ //Per tutti i vicini di u
            if(reached.contains(v)) continue; //Se il vicino è già stato visitato, lo saltiamo
            reached.insert(v); //Aggiungiamo il nodo al set dei nodi visitati
            policy.put(v); //Inseriamo il nodo
            // copiamo i componenti del ramo originale u-v, altrimenti l'albero
            // perde resistenze/generatori e i cicli stampano archi vuoti
            for (const auto& e : grafo.incident_edges(u)) {
                T other = (e.from() == u) ? e.to() : e.from();
                if (other == v) {
                    albero_risultante.add_edge(u, v, e.get_components());
                    break;
                }
            }
        }
    }

    return albero_risultante;    
}
