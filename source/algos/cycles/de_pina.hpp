#pragma once
#include "unidirected_graph/unidirected_graph.hpp"
#include "visit/graph_visit.hpp"
#include "visit/stack.hpp"
#include "solver/dijkstra.hpp"
#include <vector>
#include <list>
#include <limits>


template <typename T>
std::vector<bool> find_minimal_cycle(const UnidirectedGraph<T>& graph, const std::vector<bool>& S_i) {
    UnidirectedGraph<std::pair<T,bool>> lifted_G;

    auto s = graph.all_edges();
    std::vector<UnidirectedEdge<T>> edge_list(s.begin(), s.end());
    Component dummy_comp ("dummy", 0.0, 0); // Componente fittizio per gli archi del grafo sollevato

    for (int i = 0; i < edge_list.size(); ++i) {
        auto edge = edge_list[i];
        // Creiamo i due vertici "attivi" e "inattivi" per ogni arco del grafo originale
        T u = edge.from();
        T v = edge.to();
        // Se (u,v) è attivo in S_i, allora aggiungiamo l'arco (u+, v-) a G' e l'arco (u-, v+) a G'
        if (S_i[i]) {
            lifted_G.add_edge({u,true}, {v,false}, dummy_comp); // Archi attivi
            lifted_G.add_edge({u,false}, {v,true}, dummy_comp); 
        }
        else {
            lifted_G.add_edge({u,true}, {v,true}, dummy_comp); // Archi inattivi
            lifted_G.add_edge({u,false}, {v,false}, dummy_comp);
        }
    }

    int min_length = std::numeric_limits<int>::max(); // Lunghezza minima iniziale
    std::vector<bool> C_mu(edge_list.size(), false); // Vettore di incidenza del ciclo minimo trovato
    
    // Calcolo del cammino minimo per ogni vertice v del grafo originale tra v- e v+ in lifted_G
    for (const auto& node : graph.all_nodes()) {
        auto [dist, pred] = dijkstra(lifted_G, {node,true});
        // Impostiamo la lungheza minima del ciclo trovato come la lunghezza del cammino minimo tra v- e v+ in lifted_G
        if (dist.count({node,false}) && dist.at({node,false}) < min_length) {
            min_length = dist.at({node,false});
            C_mu = find_incidence_vector(graph, pred, {node,true}, {node,false}); // Troviamo il vettore di incidenza del ciclo trovato
        }
    }
    return C_mu;
}


// Trascrivo lo pseudocodice dell'algoritmo di De Pina per trovare i cicli essenziali in un grafo non orientato.
template<typename T>
std::vector<std::vector<bool>> De_Pina(UnidirectedGraph<T>& graph, std::vector<std::vector<bool>>& S) {
    // Salviamo il Numero di cicli essenziali
    int k = S.size();

    // Inizializziamo la matrice C per tenere traccia dei cicli essenziali
    std::vector<std::vector<bool>> C(k);

    for (int i = 0; i < k; ++i) {
        // Troviamo il Ciclo Minimo (lifting + dijkstra)
        C[i] = find_minimal_cycle(graph, S[i]);
        // Aggiorniamo i vettori S successivi
        for (int j = i + 1; j < k; ++j) {
            int scalar_product = 0;
            for (int x = 0; x < C[i].size(); ++x) { // Applichiamo il prodotto scalare tra C[i] e S[j] per verificare se fa 1
                scalar_product += C[i][x] * S[j][x];
            }
            scalar_product %= 2; // Applichiamo l'operazione modulo 2
            if (scalar_product == 1) {
                for (int x = 0; x < C[i].size(); ++x) {
                    S[j][x] = S[j][x] ^ S[i][x]; // Applichiamo la differenza simmetrica XOR tra S[j] e S[i] per aggiornare S[j]
                }
            }
        }
    }
    return C;
}



template<typename T>
void find_essential_cycles_DePina(UnidirectedGraph<T>& graph, std::vector<std::vector<UnidirectedEdge<T>>>& essential_cycles) {
    Stack<T> stack;
    UnidirectedGraph<T> support_tree = graph_visit(graph, *graph.all_nodes().begin(), stack);
    UnidirectedGraph<T> co_tree = graph - support_tree;

    auto s = graph.all_edges();
    std::vector<UnidirectedEdge<T>> edge_list(s.begin(), s.end());

    // Inizializziamo il vettore S per tenere traccia degli archi essenziali
    int k = co_tree.all_edges().size();
    std::vector<std::vector<bool>> S(k, std::vector<bool>(edge_list.size(), false));

    // Pongo 1 in posizione i per ogni arco del grafo presente nel co_tree
    int i = 0;
    for (const auto& edge : co_tree.all_edges()) {
        for (int j = 0; j < edge_list.size(); ++j) {
            if (edge_list[j] == edge) {
                S[i][j] = true; // Segniamo l'arco come presente in S[i]
                break;
            }
        }
        ++i;
    }

    // Applichiamo l'algoritmo di De Pina per trovare gli archi essenziali
    std::vector<std::vector<bool>> C = De_Pina(graph, S);

    // Traduciamo i vettori di incidenza C in cicli reali
    for (const auto& incidence_vector : C) {
        std::vector<UnidirectedEdge<T>> path_buffer; // Buffer per memorizzare gli archi del ciclo trovato
        for (int j = 0; j < incidence_vector.size(); ++j) {
            if (incidence_vector[j]) {
                path_buffer.push_back(edge_list[j]); // Aggiungiamo l'arco corrispondente al ciclo
            }
        }
        essential_cycles.push_back(path_buffer);
    }
}

