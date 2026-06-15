#pragma once
#include "unidirected_graph/unidirected_graph.hpp"
#include "cycles/dfs_based.hpp"
#include "cycles/de_pina.hpp"
#include <Eigen/Dense>
#include <vector>
#include <map>

enum class CycleType { DFS, DePina };

// Assembla le matrici del Metodo delle Correnti di Maglia:
//   R (m x m) diagonale delle resistenze, B (m x n) incidenza resistore-maglia,
//   v (n) termine noto dai generatori. m = resistori, n = cicli fondamentali.
// Righe numerate per ordine lessicografico degli archi (all_edges()).
// I cicli arrivano gia' come sequenze ordinate di nodi: il segno di B segue la
// regola PDF (arco con verso fisso nodo-minore->maggiore), +1 se la maglia
// percorre a<b, -1 se a>b. Niente euristica del nodo di partenza.
template<typename T>
void build_matrices(
    UnidirectedGraph<T>& graph,
    Eigen::MatrixXd& resistance_matrix_out,   // R
    Eigen::MatrixXd& incidence_matrix_out,    // B
    Eigen::VectorXd& voltage_vector_out,      // v
    std::vector<std::vector<T>>& fundamental_cycles_out,
    std::vector<UnidirectedEdge<T>>& resistor_branches_out,
    CycleType method = CycleType::DFS)
{
    if (method == CycleType::DFS) {
        find_essential_cycles_dfs(graph, fundamental_cycles_out);
    } else {
        find_essential_cycles_DePina(graph, fundamental_cycles_out);
    }
    const size_t n = fundamental_cycles_out.size();

    const std::set<UnidirectedEdge<T>> edge_set = graph.all_edges();
    std::vector<UnidirectedEdge<T>>& resistor_branches = resistor_branches_out;
    resistor_branches.clear();
    std::map<UnidirectedEdge<T>, size_t> row_of;
    for (const auto& e : edge_set) {
        if (e.get_component().is_resistor()) {
            row_of[e] = resistor_branches.size();
            resistor_branches.push_back(e);
        }
    }
    const size_t m = resistor_branches.size();

    resistance_matrix_out = Eigen::MatrixXd::Zero(m, m);
    for (size_t i = 0; i < m; ++i) {
        resistance_matrix_out(i, i) = resistor_branches[i].get_component().get_value();
    }

    incidence_matrix_out = Eigen::MatrixXd::Zero(m, n);
    voltage_vector_out   = Eigen::VectorXd::Zero(n);

    for (size_t j = 0; j < n; ++j) {
        const std::vector<T>& nodes = fundamental_cycles_out[j];
        const size_t k = nodes.size();

        for (size_t pos = 0; pos < k; ++pos) {
            const T a = nodes[pos];
            const T b = nodes[(pos + 1) % k];          // wrap-around chiude la maglia
            const int dir = (a < b) ? +1 : -1;         // verso maglia vs verso arco
            const UnidirectedEdge<T> key(a, b);

            auto it = edge_set.find(key);
            if (it == edge_set.end()) continue;        // difensivo: arco inesistente
            const Component& c = it->get_component();
            if (c.is_resistor()) {
                incidence_matrix_out(row_of.at(key), j) += dir;
            } else {
                // generatore: + se attraversato verso il nodo positivo
                const int vsign = (c.get_positive_node() == b) ? +1 : -1;
                voltage_vector_out(j) += vsign * c.get_value();
            }
        }
    }
}
