#pragma once
#include "unidirected_graph/unidirected_graph.hpp"
#include "visit/graph_visit.hpp"
#include "visit/stack.hpp"
#include "dijkstra.hpp"
#include <vector>
#include <list>
#include <limits>
#include <map>
#include <utility>
#include <algorithm>


// Estrae la sequenza ORDINATA dei nodi del grafo originale seguendo la catena
// pred del cammino minimo nel grafo sollevato (start = v+, end = v-).
// I nodi sollevati (T,bool) si proiettano sulla prima componente: e' esattamente
// il verso di percorrenza del ciclo minimo. start ed end proiettano sullo stesso
// nodo base v, quindi si rimuove il duplicato di chiusura finale.
// Cosi' De Pina espone i cicli gia' come cammino ordinato, senza riordini a valle.
// (Helper specifico di De Pina: vive qui, non in dijkstra.hpp, perche' conosce la
// convenzione dei nodi sollevati e della chiusura del ciclo.)
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


// Restituisce il ciclo minimo come SEQUENZA ORDINATA di nodi (verso di percorrenza),
// estratta direttamente dalla catena pred del cammino minimo nel grafo sollevato.
template <typename T>
std::vector<T> find_minimal_cycle(const UnidirectedGraph<T>& graph, const std::vector<bool>& S_i) {
    UnidirectedGraph<std::pair<T,bool>> lifted_G;

    auto s = graph.all_edges();
    std::vector<UnidirectedEdge<T>> edge_list(s.begin(), s.end());
    Component dummy_comp ("dummy", 0.0, 0); // Componente fittizio per gli archi del grafo sollevato

    for (size_t i = 0; i < edge_list.size(); ++i) {
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
    std::vector<T> best_cycle;                        // sequenza nodi del ciclo minimo

    // Calcolo del cammino minimo per ogni vertice v del grafo originale tra v- e v+ in lifted_G
    for (const auto& node : graph.all_nodes()) {
        auto [dist, pred] = dijkstra(lifted_G, {node,true});
        // Impostiamo la lungheza minima del ciclo trovato come la lunghezza del cammino minimo tra v- e v+ in lifted_G
        if (dist.count({node,false}) && dist.at({node,false}) < min_length) {
            min_length = dist.at({node,false});
            // sequenza ordinata di nodi del ciclo (verso di percorrenza)
            best_cycle = extract_cycle_nodes(pred, {node,true}, {node,false});
        }
    }
    return best_cycle;
}


// Converte un ciclo (sequenza di nodi) nel suo vettore di incidenza sugli archi,
// usato solo per il bookkeeping GF(2) di De Pina (prodotto scalare con S[j]).
template <typename T>
std::vector<bool> cycle_to_incidence(
    const std::vector<T>& nodes,
    const std::vector<UnidirectedEdge<T>>& edge_list)
{
    std::vector<bool> inc(edge_list.size(), false);
    const size_t k = nodes.size();
    for (size_t pos = 0; pos < k; ++pos) {
        UnidirectedEdge<T> e(nodes[pos], nodes[(pos + 1) % k]);   // wrap-around
        for (size_t i = 0; i < edge_list.size(); ++i)
            if (edge_list[i] == e) { inc[i] = true; break; }
    }
    return inc;
}


// Trascrivo lo pseudocodice dell'algoritmo di De Pina per trovare i cicli essenziali in un grafo non orientato.
// I cicli risultanti sono restituiti come SEQUENZE ORDINATE di nodi.
template<typename T>
std::vector<std::vector<T>> De_Pina(UnidirectedGraph<T>& graph, std::vector<std::vector<bool>>& S) {
    // Salviamo il Numero di cicli essenziali
    int k = S.size();

    // Cicli essenziali come sequenze di nodi
    std::vector<std::vector<T>> C(k);

    // edge_list per convertire un ciclo nel suo vettore di incidenza (passo GF(2))
    auto s = graph.all_edges();
    std::vector<UnidirectedEdge<T>> edge_list(s.begin(), s.end());

    for (int i = 0; i < k; ++i) {
        // Troviamo il Ciclo Minimo (lifting + dijkstra), gia' ordinato
        C[i] = find_minimal_cycle(graph, S[i]);
        const std::vector<bool> inc_i = cycle_to_incidence(C[i], edge_list);
        // Aggiorniamo i vettori S successivi
        for (int j = i + 1; j < k; ++j) {
            int scalar_product = 0;
            for (size_t x = 0; x < inc_i.size(); ++x) { // Prodotto scalare tra C[i] (incidenza) e S[j]
                scalar_product += inc_i[x] * S[j][x];
            }
            scalar_product %= 2; // Applichiamo l'operazione modulo 2
            if (scalar_product == 1) {
                for (size_t x = 0; x < inc_i.size(); ++x) {
                    S[j][x] = S[j][x] ^ S[i][x]; // Differenza simmetrica XOR tra S[j] e S[i]
                }
            }
        }
    }
    return C;
}



template<typename T>
void find_essential_cycles_DePina(UnidirectedGraph<T>& graph, std::vector<std::vector<T>>& essential_cycles) {
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
        for (size_t j = 0; j < edge_list.size(); ++j) {
            if (edge_list[j] == edge) {
                S[i][j] = true; // Segniamo l'arco come presente in S[i]
                break;
            }
        }
        ++i;
    }

    // De Pina restituisce i cicli gia' come sequenze ordinate di nodi
    // (verso di percorrenza dal cammino di Dijkstra): nessun riordino a valle.
    essential_cycles = De_Pina(graph, S);
}

