// Test dell'individuazione delle maglie (cicli fondamentali): conteggio,
// validita' come cammini chiusi, accordo fra DFS e De Pina.
#include "test_harness.hpp"

#include <algorithm>
#include <list>

static bool adjacent(const UnidirectedGraph<int>& g, int a, int b) {
    const std::list<int> n = g.neighbours(a);
    return std::find(n.begin(), n.end(), b) != n.end();
}

// Numero di cicli fondamentali = |E| - |V| + 1, per entrambi i metodi.
static void test_cycle_count() {
    std::cout << "[cycles] conteggio |E|-|V|+1\n";
    for (const char* nl : {"example_sec7.txt", "two_mesh.txt", "series_loop.txt"}) {
        for (auto [m, mn] : std::vector<std::pair<CycleType, const char*>>{
                 {CycleType::DFS, "DFS"}, {CycleType::DePina, "DePina"}}) {
            Solution s = solve(nl, m);
            const size_t expected = s.n_edges - s.n_nodes + 1;
            CHECK(s.cycles.size() == expected,
                  std::string(nl) + "/" + mn + ": #maglie = |E|-|V|+1");
        }
    }
}

// Ogni maglia e' un cammino chiuso: nodi consecutivi adiacenti nel grafo, e la
// corda di chiusura (wrap-around ultimo->primo) e' anch'essa un arco esistente.
static void test_cycles_are_closed_walks() {
    std::cout << "[cycles] maglie = cammini chiusi validi\n";
    for (const char* nl : {"example_sec7.txt", "two_mesh.txt", "series_loop.txt"}) {
        for (auto m : {CycleType::DFS, CycleType::DePina}) {
            UnidirectedGraph<int> g = load(nl);
            Solution s = solve(nl, m);
            for (const auto& cyc : s.cycles) {
                CHECK(cyc.size() >= 2, std::string(nl) + ": maglia non degenere");
                const size_t k = cyc.size();
                for (size_t p = 0; p < k; ++p) {
                    const int a = cyc[p];
                    const int b = cyc[(p + 1) % k];   // wrap-around
                    CHECK(adjacent(g, a, b),
                          std::string(nl) + ": arco di maglia esistente");
                }
            }
        }
    }
}

// DFS e De Pina sono basi di cicli diverse, ma le tensioni/correnti FISICHE sui
// resistori (verso di riferimento fisso nodo-min->nodo-max) devono coincidere.
static void test_methods_agree() {
    std::cout << "[cycles] accordo DFS vs De Pina sulle grandezze fisiche\n";
    for (const char* nl : {"example_sec7.txt", "two_mesh.txt", "series_loop.txt"}) {
        Solution a = solve(nl, CycleType::DFS);
        Solution b = solve(nl, CycleType::DePina);
        CHECK(a.cycles.size() == b.cycles.size(),
              std::string(nl) + ": stesso n. maglie");
        CHECK(a.volt.size() == b.volt.size(),
              std::string(nl) + ": stesso n. resistori");
        for (const auto& [name, vA] : a.volt) {
            CHECK(b.volt.count(name) == 1, "resistore in entrambi: " + name);
            CHECK_NEAR(vA, b.volt[name], 1e-6, "V " + name + " DFS==DePina");
            CHECK_NEAR(a.curr[name], b.curr[name], 1e-6, "I " + name + " DFS==DePina");
        }
    }
}

int main() {
    test_cycle_count();
    test_cycles_are_closed_walks();
    test_methods_agree();
    return report("cycles");
}
