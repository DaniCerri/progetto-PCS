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
template<typename T>
void build_matrices(
    UnidirectedGraph<T>& graph,
    Eigen::MatrixXd& resistance_matrix_out,   // R
    Eigen::MatrixXd& incidence_matrix_out,    // B
    Eigen::VectorXd& voltage_vector_out,      // v
    std::vector<std::vector<UnidirectedEdge<T>>>& fundamental_cycles_out,
    std::vector<UnidirectedEdge<T>>& resistor_branches_out,// riga i -> arco resistore
    CycleType method = CycleType::DFS)  
{

    // 1. Cicli fondamentali (archi normalizzati from<to)
    if (method == CycleType::DFS) {
        find_essential_cycles_dfs(graph, fundamental_cycles_out);
    } else {
        find_essential_cycles_DePina(graph, fundamental_cycles_out);
    }
    const size_t n = fundamental_cycles_out.size();

    // 2. numerazione resistori: archi il cui componente e' un resistore, ordine lessicografico
    std::vector<UnidirectedEdge<T>>& resistor_branches = resistor_branches_out;  // riga i -> arco
    resistor_branches.clear();
    std::map<UnidirectedEdge<T>, size_t> row_of;         // arco  -> riga
    for (const auto& e : graph.all_edges()) {
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

    // 4. B e v percorrendo ogni ciclo nel suo verso
    incidence_matrix_out = Eigen::MatrixXd::Zero(m, n);
    voltage_vector_out   = Eigen::VectorXd::Zero(n);

    for (size_t j = 0; j < n; ++j) {
        const auto& cycle = fundamental_cycles_out[j];

        // nodo di partenza: estremo di cycle[0] condiviso con l'ultimo arco
        const auto& first = cycle.front();
        const auto& last  = cycle.back();
        T cur = (first.from() == last.from() || first.from() == last.to())
                    ? first.from() : first.to();

        for (const auto& e : cycle) {
            const int dir = (e.from() == cur) ? +1 : -1;          // +1 se percorso from->to
            const T   nxt = (e.from() == cur) ? e.to() : e.from();
            const Component& c = e.get_component();
            if (c.is_resistor()) {
                // riga del resistore: dir e' il segno in B
                incidence_matrix_out(row_of.at(e), j) += dir;
            } else {
                // generatore: contributo + se attraversato da "-" a "+"
                // (usciamo dall'arco nel nodo positivo)
                const int vsign = (c.get_positive_node() == nxt) ? +1 : -1;
                voltage_vector_out(j) += vsign * c.get_value();
            }
            cur = nxt;
        }
    }
}
