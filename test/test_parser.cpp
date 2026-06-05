// Test del parser della netlist: robustezza alla spaziatura e gestione errori.
#include "test_harness.hpp"

// Spazi multipli, tab e righe vuote non devono cambiare il grafo costruito.
static void test_robustness() {
    std::cout << "[parser] robustezza spazi/tab/righe vuote\n";
    UnidirectedGraph<int> clean = load("example_sec7.txt");
    UnidirectedGraph<int> messy = load("messy_sec7.txt");

    CHECK(clean.all_edges().size() == messy.all_edges().size(), "stesso n. archi");
    CHECK(clean.all_nodes().size() == messy.all_nodes().size(), "stessi nodi");

    const auto a = components_of(clean);
    const auto b = components_of(messy);
    CHECK(a.size() == b.size(), "stesso n. componenti");
    for (const auto& [name, val] : a) {
        CHECK(b.count(name) == 1, "componente presente: " + name);
        CHECK_NEAR(b.at(name), val, 1e-12, "valore " + name);
    }
}

// La prima colonna (tipo) decide R/V: verifichiamo la classificazione.
static void test_type_classification() {
    std::cout << "[parser] classificazione tipo R/V\n";
    UnidirectedGraph<int> g = load("example_sec7.txt");
    int resistori = 0, generatori = 0;
    for (const auto& e : g.all_edges()) {
        if (e.get_component().is_resistor()) ++resistori;
        else ++generatori;
    }
    CHECK(resistori == 5, "5 resistori");
    CHECK(generatori == 2, "2 generatori");
}

// Input invalido -> eccezione, niente crash silenzioso.
static void test_errors() {
    std::cout << "[parser] errori: malformato / tipo / file mancante\n";
    CHECK(throws([] { load("malformed.txt"); }), "riga a 3 token -> throw");
    CHECK(throws([] { load("bad_type.txt"); }), "tipo non R/V -> throw");
    CHECK(throws([] { load("NON_ESISTE.txt"); }), "file mancante -> throw");
}

int main() {
    test_robustness();
    test_type_classification();
    test_errors();
    return report("parser");
}
