// Suite di test del solver per il Metodo delle Correnti di Maglia.
// Niente framework esterno (solo Eigen, gia' dipendenza del progetto): un piccolo
// harness con macro CHECK/CHECK_NEAR conta i fallimenti e ritorna != 0 se almeno
// uno fallisce -> integrabile in CTest (add_test).
//
// Strategia di copertura:
//   * GOLDEN  : valori attesi presi dalla SPECIFICA (sez. 7 del PDF e esempio
//               a 2 maglie), indipendenti dall'implementazione.
//   * PROPRIETA': invarianti che non richiedono valori calcolati a mano
//               (conteggio cicli fondamentali |E|-|V|+1, accordo DFS vs De Pina,
//               residuo del sistema lineare ~ 0, correnti uguali in serie).
//   * PARSER  : robustezza a spazi/tab/righe vuote e fallimento pulito su input
//               malformato / tipo non valido / file mancante.

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

// ---- helper di soluzione (replica la pipeline del main, senza stampe) --------

struct Solution {
    std::vector<std::vector<int>> cycles;
    Eigen::MatrixXd R, B;
    Eigen::VectorXd v, i;
    std::vector<UnidirectedEdge<int>> resistors;
    std::map<std::string, double> volt;   // nome resistore -> tensione
    std::map<std::string, double> curr;   // nome resistore -> corrente
    size_t n_edges = 0, n_nodes = 0;
};

static UnidirectedGraph<int> load(const std::string& name) {
    Parser p;
    UnidirectedGraph<int> g;
    std::string path = std::string(TEST_DIR) + "/" + name;
    p.pipeline(path, g);
    return g;
}

static Solution solve(const std::string& name, CycleType method) {
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
static bool throws(F f) {
    try {
        f();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

// ---- test -------------------------------------------------------------------

// GOLDEN: netlist e risultati della sez. 7 del PDF, per entrambi i metodi.
static void test_golden_sec7(CycleType m, const char* mname) {
    std::cout << "[golden sez.7 / " << mname << "]\n";
    Solution s = solve("example_sec7.txt", m);

    CHECK(s.cycles.size() == s.n_edges - s.n_nodes + 1, "conteggio maglie");
    CHECK(s.cycles.size() == 3, "3 maglie attese (7 archi, 5 nodi)");
    CHECK(s.resistors.size() == 5, "5 resistori");

    CHECK_NEAR(s.volt["R1"], 8.0, 1e-6, "V_R1");
    CHECK_NEAR(s.curr["R1"], 2.0, 1e-6, "I_R1");
    CHECK_NEAR(s.volt["R2"], 22.0, 1e-6, "V_R2");
    CHECK_NEAR(s.curr["R2"], 2.2, 1e-6, "I_R2");
    CHECK_NEAR(s.volt["R3"], -6.0, 1e-6, "V_R3");
    CHECK_NEAR(s.curr["R3"], -0.2, 1e-6, "I_R3");
    CHECK_NEAR(s.volt["R4"], -28.0, 1e-6, "V_R4");
    CHECK_NEAR(s.curr["R4"], -2.8, 1e-6, "I_R4");
    CHECK_NEAR(s.volt["R5"], 12.0, 1e-6, "V_R5");
    CHECK_NEAR(s.curr["R5"], 3.0, 1e-6, "I_R5");
}

// GOLDEN: esempio a 2 maglie del PDF (sez. 4). I segni dipendono dalle maglie
// scelte, quindi confronto i MODULI delle tensioni: 10/11, 100/11, 120/11.
static void test_golden_two_mesh(CycleType m, const char* mname) {
    std::cout << "[golden 2-maglie / " << mname << "]\n";
    Solution s = solve("two_mesh.txt", m);

    CHECK(s.cycles.size() == 2, "2 maglie attese (5 archi, 4 nodi)");
    CHECK_NEAR(std::abs(s.volt["R1"]), 10.0 / 11.0, 1e-4, "|V_R1|");
    CHECK_NEAR(std::abs(s.volt["R2"]), 100.0 / 11.0, 1e-4, "|V_R2|");
    CHECK_NEAR(std::abs(s.volt["R3"]), 120.0 / 11.0, 1e-4, "|V_R3|");
}

// GOLDEN/PROPRIETA': maglia singola in serie V=12, R1=1, R2=2 -> I=4 A,
// stessa corrente (in modulo) in entrambi i resistori, V_Rk = Rk * I.
static void test_series_loop(CycleType m, const char* mname) {
    std::cout << "[serie singola maglia / " << mname << "]\n";
    Solution s = solve("series_loop.txt", m);

    CHECK(s.cycles.size() == 1, "1 maglia");
    CHECK_NEAR(std::abs(s.curr["R1"]), 4.0, 1e-6, "|I_R1| = 12/(1+2)");
    CHECK_NEAR(std::abs(s.curr["R2"]), 4.0, 1e-6, "|I_R2| = stessa corrente in serie");
    CHECK_NEAR(std::abs(s.volt["R1"]), 4.0, 1e-6, "|V_R1| = 1*4");
    CHECK_NEAR(std::abs(s.volt["R2"]), 8.0, 1e-6, "|V_R2| = 2*4");
}

// PROPRIETA': DFS e De Pina sono basi di cicli diverse ma le tensioni FISICHE
// sui resistori (verso di riferimento fisso nodo-min->nodo-max) devono coincidere.
static void test_methods_agree(const std::string& netlist) {
    std::cout << "[accordo DFS vs De Pina / " << netlist << "]\n";
    Solution a = solve(netlist, CycleType::DFS);
    Solution b = solve(netlist, CycleType::DePina);

    CHECK(a.cycles.size() == b.cycles.size(), "stesso numero di maglie");
    CHECK(a.volt.size() == b.volt.size(), "stesso numero di resistori");
    for (const auto& [name, vA] : a.volt) {
        CHECK(b.volt.count(name) == 1, "resistore presente in entrambi: " + name);
        CHECK_NEAR(vA, b.volt[name], 1e-6, "V " + name + " DFS==DePina");
        CHECK_NEAR(a.curr[name], b.curr[name], 1e-6, "I " + name + " DFS==DePina");
    }
}

// PROPRIETA': il CG risolve davvero B^T R B i = v -> residuo ~ 0.
static void test_residual(const std::string& netlist, CycleType m) {
    Solution s = solve(netlist, m);
    const Eigen::MatrixXd A = s.B.transpose() * s.R * s.B;
    const double res = (A * s.i - s.v).norm();
    CHECK(res < 1e-6, "residuo ||A i - v|| ~ 0 per " + netlist);
}

// PARSER: spazi multipli, tab e righe vuote non cambiano la soluzione.
static void test_parser_robustness() {
    std::cout << "[robustezza parser: spazi/tab/righe vuote]\n";
    Solution clean = solve("example_sec7.txt", CycleType::DFS);
    Solution messy = solve("messy_sec7.txt", CycleType::DFS);

    CHECK(clean.resistors.size() == messy.resistors.size(), "stessi resistori");
    for (const auto& [name, v] : clean.volt) {
        CHECK_NEAR(messy.volt[name], v, 1e-6, "V " + name + " robusto a spaziatura");
    }
}

// PARSER: input invalido -> eccezione (niente crash silenzioso).
static void test_parser_errors() {
    std::cout << "[errori parser: malformato / tipo / file mancante]\n";
    CHECK(throws([] { load("malformed.txt"); }), "riga a 3 token -> throw");
    CHECK(throws([] { load("bad_type.txt"); }), "tipo non R/V -> throw");
    CHECK(throws([] { load("NON_ESISTE.txt"); }), "file mancante -> throw");
}

// PROPRIETA': add_edge rifiuta un secondo componente sullo stesso arco (no rami
// in parallelo, come da specifica).
static void test_parallel_edge_rejected() {
    std::cout << "[arco duplicato rifiutato]\n";
    UnidirectedGraph<int> g;
    g.add_edge(1, 2, Component("R1", 10, 1));
    CHECK(throws([&] { g.add_edge(1, 2, Component("R2", 20, 1)); }),
          "secondo componente su (1,2) -> throw");
}

int main() {
    for (auto [m, name] : std::vector<std::pair<CycleType, const char*>>{
             {CycleType::DFS, "DFS"}, {CycleType::DePina, "DePina"}}) {
        test_golden_sec7(m, name);
        test_golden_two_mesh(m, name);
        test_series_loop(m, name);
    }

    for (const char* nl : {"example_sec7.txt", "two_mesh.txt", "series_loop.txt"}) {
        test_methods_agree(nl);
        test_residual(nl, CycleType::DFS);
        test_residual(nl, CycleType::DePina);
    }

    test_parser_robustness();
    test_parser_errors();
    test_parallel_edge_rejected();

    std::cout << "\n" << (g_checks - g_fail) << "/" << g_checks
              << " check superati.\n";
    if (g_fail) {
        std::cerr << g_fail << " CHECK FALLITI\n";
        return 1;
    }
    std::cout << "TUTTI I TEST OK\n";
    return 0;
}
