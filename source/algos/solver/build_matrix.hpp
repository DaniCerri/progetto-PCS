#pragma once
#include "unidirected_graph/unidirected_graph.hpp"
#include "cycles/dfs_based.hpp"
#include <Eigen/Dense>
#include <vector>
#include <map>

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
// resistor_branches[i] e' l'arco stesso, .get_components() i suoi componenti interni.
template<typename T>
void build_matrices(
    UnidirectedGraph<T>& graph,
    Eigen::MatrixXd& resistance_matrix_out,   // R
    Eigen::MatrixXd& incidence_matrix_out,    // B
    Eigen::VectorXd& voltage_vector_out,      // v
    std::vector<std::vector<UnidirectedEdge<T>>>& fundamental_cycles_out,
    std::vector<UnidirectedEdge<T>>& resistor_branches_out)  // riga i -> arco resistore
{
    // 1. cicli fondamentali (archi normalizzati from<to)
    // TODO: generalizzare per De Pina
    find_essential_cycles_dfs(graph, fundamental_cycles_out);
    const size_t n = fundamental_cycles_out.size();

    // 2. numerazione resistori: rami con almeno un resistore, ordine lessicografico
    std::vector<UnidirectedEdge<T>>& resistor_branches = resistor_branches_out;  // riga i -> arco
    resistor_branches.clear();
    std::map<UnidirectedEdge<T>, size_t> row_of;         // arco  -> riga
    for (const auto& e : graph.all_edges()) {
        bool has_resistor = false;
        for (const auto& c : e.get_components())
            if (c.is_resistor()) { has_resistor = true; break; }
        if (has_resistor) {
            row_of[e] = resistor_branches.size();
            resistor_branches.push_back(e);
        }
    }
    const size_t m = resistor_branches.size();

    // 3. R diagonale (somma dei resistori in serie sullo stesso ramo)
    resistance_matrix_out = Eigen::MatrixXd::Zero(m, m);
    for (size_t i = 0; i < m; ++i) {
        double r = 0.0;
        for (const auto& c : resistor_branches[i].get_components())
            if (c.is_resistor()) r += c.get_value();
        resistance_matrix_out(i, i) = r;
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

            for (const auto& c : e.get_components()) {
                if (c.is_resistor()) {
                    // riga del resistore: dir e' il segno in B
                    incidence_matrix_out(row_of.at(e), j) += dir;
                } else {
                    // generatore: contributo + se attraversato da "-" a "+"
                    // (usciamo dall'arco nel nodo positivo)
                    const int vsign = (c.get_positive_node() == nxt) ? +1 : -1;
                    voltage_vector_out(j) += vsign * c.get_value();
                }
            }
            cur = nxt;
        }
    }
}
