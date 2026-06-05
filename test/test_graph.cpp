// Test della struttura dati UnidirectedGraph: costruzione, vicinato, incidenza,
// rifiuto archi duplicati, differenza fra grafi.
#include "test_harness.hpp"

#include <algorithm>
#include <list>

static bool list_contains(const std::list<int>& l, int x) {
    return std::find(l.begin(), l.end(), x) != l.end();
}

// add_edge inserisce automaticamente i nodi mancanti e normalizza from<to.
static void test_construction() {
    std::cout << "[graph] costruzione e normalizzazione archi\n";
    UnidirectedGraph<int> g;
    g.add_edge(2, 1, Component("R1", 10, 2));   // passo (2,1): deve normalizzare a (1,2)
    g.add_edge(2, 3, Component("R2", 20, 2));

    CHECK(g.all_nodes().size() == 3, "3 nodi (1,2,3)");
    CHECK(g.all_edges().size() == 2, "2 archi");

    const auto edges = g.all_edges();
    const UnidirectedEdge<int> e = *edges.begin();   // set ordinato: primo = (1,2)
    CHECK(e.from() == 1 && e.to() == 2, "arco normalizzato from<to");
}

// Un secondo componente sullo stesso arco (ramo in parallelo) va rifiutato.
static void test_duplicate_rejected() {
    std::cout << "[graph] arco duplicato rifiutato\n";
    UnidirectedGraph<int> g;
    g.add_edge(1, 2, Component("R1", 10, 1));
    CHECK(throws([&] { g.add_edge(1, 2, Component("R2", 20, 1)); }),
          "secondo componente su (1,2) -> throw");
    CHECK(throws([&] { g.add_edge(2, 1, Component("R3", 30, 2)); }),
          "duplicato anche con nodi invertiti -> throw");
}

// neighbours e incident_edges sulla lista di adiacenza.
static void test_adjacency() {
    std::cout << "[graph] vicinato e archi incidenti\n";
    UnidirectedGraph<int> g;
    g.add_edge(1, 2, Component("R1", 1, 1));
    g.add_edge(2, 3, Component("R2", 1, 2));
    g.add_edge(2, 4, Component("R3", 1, 2));

    const std::list<int> n2 = g.neighbours(2);
    CHECK(n2.size() == 3, "il nodo 2 ha grado 3");
    CHECK(list_contains(n2, 1) && list_contains(n2, 3) && list_contains(n2, 4),
          "vicini di 2 = {1,3,4}");
    CHECK(g.neighbours(1).size() == 1, "il nodo 1 ha grado 1");
    CHECK(g.incident_edges(2).size() == 3, "3 archi incidenti a 2");
    CHECK(g.neighbours(99).empty(), "nodo inesistente -> nessun vicino");
}

// edge_number / edge_at: round-trip indice<->arco, e indice oltre il range.
static void test_indexing() {
    std::cout << "[graph] numerazione archi\n";
    UnidirectedGraph<int> g;
    g.add_edge(1, 2, Component("R1", 1, 1));
    g.add_edge(2, 3, Component("R2", 1, 2));

    const UnidirectedEdge<int> key(1, 2);
    const size_t idx = g.edge_number(key);
    CHECK(idx < g.all_edges().size(), "arco (1,2) trovato");
    CHECK(g.edge_at(idx) == key, "edge_at(edge_number(e)) == e");

    const UnidirectedEdge<int> absent(7, 8);
    CHECK(g.edge_number(absent) == 2, "arco assente -> ritorna edges.size()");
}

// operator-: differenza fra grafo e suo sottoinsieme (usato per il co-albero).
static void test_difference() {
    std::cout << "[graph] differenza fra grafi (co-albero)\n";
    UnidirectedGraph<int> g;
    g.add_edge(1, 2, Component("R1", 1, 1));
    g.add_edge(2, 3, Component("R2", 1, 2));
    g.add_edge(1, 3, Component("R3", 1, 1));   // chiude il triangolo

    UnidirectedGraph<int> tree;
    tree.add_edge(1, 2, Component("R1", 1, 1));
    tree.add_edge(2, 3, Component("R2", 1, 2));

    UnidirectedGraph<int> co = g - tree;
    CHECK(co.all_edges().size() == 1, "1 corda nel co-albero");
    const UnidirectedEdge<int> chord = *co.all_edges().begin();
    CHECK(chord == UnidirectedEdge<int>(1, 3), "la corda e' (1,3)");
}

int main() {
    test_construction();
    test_duplicate_rejected();
    test_adjacency();
    test_indexing();
    test_difference();
    return report("graph");
}
