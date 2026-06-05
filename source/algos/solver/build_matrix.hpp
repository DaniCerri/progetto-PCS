#pragma once
#include "unidirected_graph/unidirected_graph.hpp"
#include "cycles/dfs_based.hpp"
#include "cycles/de_pina.hpp"
#include <Eigen/Dense>
#include <vector>
#include <map>

// L'enum è globale e visibile anche dal main.cpp
enum class CycleType { DFS, DePina };

// Assembla le matrici del Metodo delle Correnti di Maglia:
//   R  (m x m) diagonale delle resistenze
//   B  (m x n) matrice di incidenza resistore-maglia (+1/-1/0)
//   v  (n)     termine noto dai generatori
// dove m = numero di resistori, n = numero di cicli fondamentali.
//
// Numerazione delle righe (resistori): ordine lessicografico degli archi.
// all_edges() restituisce uno std::set -> gia' ordinato per (from,to),
// e il costruttore di UnidirectedEdge forza from<to (verso nodo minore->maggiore,
// come richiesto dalla specifica). row_of lega arco -> riga in O(log m);
// modello a singolo componente per arco: resistor_branches[i] e' l'arco stesso,
// .get_component() il suo unico componente (resistore).
//
// I cicli fondamentali arrivano gia' come SEQUENZE ORDINATE di nodi (verso di
// percorrenza), da entrambi i metodi (DFS e De Pina). Questo rende il segno di B
// una pura applicazione della regola PDF: l'arco ha verso di riferimento fisso
// nodo-minore->nodo-maggiore, quindi se la maglia percorre l'arco a->b il segno e'
// +1 quando a<b (verso concorde), -1 quando a>b. Niente piu' euristica del nodo
// di partenza ne' ricamminata degli archi (che era fragile e, su cicli >=4 archi
// non ordinati come quelli di De Pina, produceva segni incoerenti).
template<typename T>
void build_matrices(
    UnidirectedGraph<T>& graph,
    Eigen::MatrixXd& resistance_matrix_out,   // R
    Eigen::MatrixXd& incidence_matrix_out,    // B
    Eigen::VectorXd& voltage_vector_out,      // v
    std::vector<std::vector<T>>& fundamental_cycles_out,   // maglie = sequenze di nodi
    std::vector<UnidirectedEdge<T>>& resistor_branches_out,// riga i -> arco resistore
    CycleType method = CycleType::DFS)
{

    // 1. Cicli fondamentali come sequenze ordinate di nodi
    if (method == CycleType::DFS) {
        find_essential_cycles_dfs(graph, fundamental_cycles_out);
    } else {
        find_essential_cycles_DePina(graph, fundamental_cycles_out);
    }
    const size_t n = fundamental_cycles_out.size();

    // 2. archi in ordine lessicografico (std::set gia' ordinato per (from,to)):
    //    definisce la numerazione delle righe; i resistori prendono le righe di B/R.
    //    edge_set serve anche per risalire dall'arco (a,b) al suo componente.
    const std::set<UnidirectedEdge<T>> edge_set = graph.all_edges();
    std::vector<UnidirectedEdge<T>>& resistor_branches = resistor_branches_out;  // riga i -> arco
    resistor_branches.clear();
    std::map<UnidirectedEdge<T>, size_t> row_of;         // arco resistore -> riga
    for (const auto& e : edge_set) {
        if (e.get_component().is_resistor()) {
            row_of[e] = resistor_branches.size();
            resistor_branches.push_back(e);
        }
    }
    const size_t m = resistor_branches.size();

    // 3. R diagonale: un resistore per arco -> R(i,i) = valore del componente
    resistance_matrix_out = Eigen::MatrixXd::Zero(m, m);
    for (size_t i = 0; i < m; ++i) {
        resistance_matrix_out(i, i) = resistor_branches[i].get_component().get_value();
    }

    // 4. B e v percorrendo ogni maglia secondo l'ordine dei suoi nodi
    incidence_matrix_out = Eigen::MatrixXd::Zero(m, n);
    voltage_vector_out   = Eigen::VectorXd::Zero(n);

    for (size_t j = 0; j < n; ++j) {
        const std::vector<T>& nodes = fundamental_cycles_out[j];
        const size_t k = nodes.size();

        for (size_t pos = 0; pos < k; ++pos) {
            const T a = nodes[pos];
            const T b = nodes[(pos + 1) % k];          // wrap-around chiude la maglia
            const int dir = (a < b) ? +1 : -1;         // verso maglia vs verso arco (regola PDF)
            const UnidirectedEdge<T> key(a, b);        // costruttore normalizza from<to

            auto it = edge_set.find(key);
            if (it == edge_set.end()) continue;        // difensivo: arco inesistente
            const Component& c = it->get_component();
            if (c.is_resistor()) {
                // riga del resistore: dir e' il segno in B
                incidence_matrix_out(row_of.at(key), j) += dir;
            } else {
                // generatore: contributo + se attraversato da "-" a "+"
                // (usciamo dall'arco nel nodo b: se b e' il positivo -> +)
                const int vsign = (c.get_positive_node() == b) ? +1 : -1;
                voltage_vector_out(j) += vsign * c.get_value();
            }
        }
    }
}
