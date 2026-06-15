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


// Segue la catena pred del cammino minimo nel grafo sollevato (start=v+, end=v-)
// proiettando i nodi (T,bool) sulla prima componente: e' il verso di percorrenza
// del ciclo. start ed end proiettano sullo stesso nodo base -> tolgo il doppione.
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
    if (nodes.size() >= 2 && nodes.front() == nodes.back())
        nodes.pop_back();
    return nodes;
}


// Ciclo minimo come sequenza ordinata di nodi, dalla catena pred del cammino
// minimo nel grafo sollevato.
template <typename T>
std::vector<T> find_minimal_cycle(const UnidirectedGraph<T>& graph, const std::vector<bool>& S_i) {
    UnidirectedGraph<std::pair<T,bool>> lifted_G;

    auto s = graph.all_edges();
    std::vector<UnidirectedEdge<T>> edge_list(s.begin(), s.end());
    Component dummy_comp ("dummy", 0.0, 0);

    for (size_t i = 0; i < edge_list.size(); ++i) {
        auto edge = edge_list[i];
        T u = edge.from();
        T v = edge.to();
        // arco attivo in S_i -> lifting incrociato (u+,v-)/(u-,v+); altrimenti diretto
        if (S_i[i]) {
            lifted_G.add_edge({u,true}, {v,false}, dummy_comp);
            lifted_G.add_edge({u,false}, {v,true}, dummy_comp);
        }
        else {
            lifted_G.add_edge({u,true}, {v,true}, dummy_comp);
            lifted_G.add_edge({u,false}, {v,false}, dummy_comp);
        }
    }

    int min_length = std::numeric_limits<int>::max();
    std::vector<T> best_cycle;

    // ciclo minimo = cammino minimo tra v- e v+ nel grafo sollevato, su tutti i v
    for (const auto& node : graph.all_nodes()) {
        auto [dist, pred] = dijkstra(lifted_G, {node,true});
        if (dist.count({node,false}) && dist.at({node,false}) < min_length) {
            min_length = dist.at({node,false});
            best_cycle = extract_cycle_nodes(pred, {node,true}, {node,false});
        }
    }
    return best_cycle;
}


// Vettore di incidenza del ciclo sugli archi, per il bookkeeping GF(2) di De Pina.
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


// Algoritmo di De Pina per la base dei cicli minimi: i cicli sono restituiti come
// sequenze ordinate di nodi.
template<typename T>
std::vector<std::vector<T>> De_Pina(UnidirectedGraph<T>& graph, std::vector<std::vector<bool>>& S) {
    int k = S.size();
    std::vector<std::vector<T>> C(k);

    auto s = graph.all_edges();
    std::vector<UnidirectedEdge<T>> edge_list(s.begin(), s.end());

    for (int i = 0; i < k; ++i) {
        C[i] = find_minimal_cycle(graph, S[i]);
        const std::vector<bool> inc_i = cycle_to_incidence(C[i], edge_list);
        // aggiorno gli S successivi: se <C[i], S[j]> dispari (mod 2) -> S[j] ^= S[i]
        for (int j = i + 1; j < k; ++j) {
            int scalar_product = 0;
            for (size_t x = 0; x < inc_i.size(); ++x) {
                scalar_product += inc_i[x] * S[j][x];
            }
            scalar_product %= 2;
            if (scalar_product == 1) {
                for (size_t x = 0; x < inc_i.size(); ++x) {
                    S[j][x] = S[j][x] ^ S[i][x];
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

    // S[i] = vettore di incidenza dell'i-esimo arco del co-albero
    int k = co_tree.all_edges().size();
    std::vector<std::vector<bool>> S(k, std::vector<bool>(edge_list.size(), false));

    int i = 0;
    for (const auto& edge : co_tree.all_edges()) {
        for (size_t j = 0; j < edge_list.size(); ++j) {
            if (edge_list[j] == edge) {
                S[i][j] = true;
                break;
            }
        }
        ++i;
    }

    essential_cycles = De_Pina(graph, S);
}
