// Harness di test condiviso fra i vari file (parser, grafo, cicli, solver).
// Niente framework esterno: macro CHECK/CHECK_NEAR su contatori a linkage interno
// (uno per file di test) + helper comuni. Ogni file di test ha il suo main() che
// chiama report() come valore di ritorno (0 = tutti i check ok, 1 = fallimenti),
// cosi' CTest tratta ciascuna area come un test separato.
#pragma once

#include "parser/parser.hpp"
#include "unidirected_graph/unidirected_graph.hpp"
#include "solver/build_matrix.hpp"
#include "solver/gradiente_coniugato.hpp"

#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifndef TEST_DIR
#define TEST_DIR "."
#endif

static int g_fail = 0;
static int g_checks = 0;

#define CHECK(cond, msg)                                                        \
    do {                                                                        \
        ++g_checks;                                                             \
        if (!(cond)) {                                                          \
            ++g_fail;                                                           \
            std::cerr << "  [FAIL] " << msg << "  (" #cond ")  @L" << __LINE__   \
                      << "\n";                                                  \
        }                                                                       \
    } while (0)

#define CHECK_NEAR(got, exp, tol, msg)                                          \
    do {                                                                        \
        ++g_checks;                                                             \
        const double _d = std::abs((got) - (exp));                             \
        if (!(_d <= (tol))) {                                                    \
            ++g_fail;                                                           \
            std::cerr << "  [FAIL] " << msg << "  atteso " << (exp) << " ott. "  \
                      << (got) << " (|d|=" << _d << ")  @L" << __LINE__ << "\n"; \
        }                                                                       \
    } while (0)

// Stampa il riepilogo e ritorna il codice d'uscita per main().
static inline int report(const char* suite) {
    std::cout << "[" << suite << "] " << (g_checks - g_fail) << "/" << g_checks
              << " check superati.\n";
    if (g_fail) {
        std::cerr << "[" << suite << "] " << g_fail << " CHECK FALLITI\n";
        return 1;
    }
    return 0;
}

// ---- helper comuni ----------------------------------------------------------

// Carica una netlist da test/netlists/ e ritorna il grafo costruito.
static inline UnidirectedGraph<int> load(const std::string& name) {
    Parser p;
    UnidirectedGraph<int> g;
    std::string path = std::string(TEST_DIR) + "/" + name;
    p.pipeline(path, g);
    return g;
}

// nome componente -> valore, per confronti a livello di parsing/grafo.
static inline std::map<std::string, double>
components_of(const UnidirectedGraph<int>& g) {
    std::map<std::string, double> m;
    for (const auto& e : g.all_edges())
        m[e.get_component().get_name()] = e.get_component().get_value();
    return m;
}

// Soluzione completa del circuito (replica la pipeline del main, senza stampe).
struct Solution {
    std::vector<std::vector<int>> cycles;
    Eigen::MatrixXd R, B;
    Eigen::VectorXd v, i;
    std::vector<UnidirectedEdge<int>> resistors;
    std::map<std::string, double> volt;   // nome resistore -> tensione
    std::map<std::string, double> curr;   // nome resistore -> corrente
    size_t n_edges = 0, n_nodes = 0;
};

static inline Solution solve(const std::string& name, CycleType method) {
    Solution s;
    UnidirectedGraph<int> g = load(name);
    s.n_edges = g.all_edges().size();
    s.n_nodes = g.all_nodes().size();

    build_matrices(g, s.R, s.B, s.v, s.cycles, s.resistors, method);

    s.i = Eigen::VectorXd::Zero(static_cast<Eigen::Index>(s.cycles.size()));
    gradiente_coniugato(s.B.transpose() * s.R * s.B, s.v, s.i, 1e-12);

    const Eigen::VectorXd branch = s.B * s.i;   // I per ramo resistivo
    for (size_t k = 0; k < s.resistors.size(); ++k) {
        const Component& c = s.resistors[k].get_component();
        s.curr[c.get_name()] = branch(static_cast<Eigen::Index>(k));
        s.volt[c.get_name()] = c.get_value() * branch(static_cast<Eigen::Index>(k));
    }
    return s;
}

template <typename F>
static inline bool throws(F f) {
    try {
        f();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}
