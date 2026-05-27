#include "parser/parser.hpp"
#include "unidirected_graph/unidirected_graph.hpp"
#include <iostream>
#include <fstream>
#include "cycles/dfs_based.hpp"

void salva_dot(const std::string& nome_file, const UnidirectedGraph<int>& circuito) {
    std::ofstream file(nome_file);
    circuito.to_dot(file, "Circuito");
}

void salva_tikz_dot(const std::string& nome_file, const UnidirectedGraph<int>& circuito) {
    std::ofstream file(nome_file);
    circuito.to_tikz_dot(file, "Circuito");
}

int main (const int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Utilizzo: " << argv[0] << " <file_input> <file_output>" << std::endl;
        return 1;
    }

    std::string file_input = argv[1];
    std::string file_output = argv[2];

    Parser parser;
    UnidirectedGraph<int> circuito;

    // leggo la netlist
    parser.pipeline(file_input, circuito);

    // visualizzo il circuito
    salva_dot(file_output, circuito);
    // salvo anche la topologia-only per la pipeline CircuiTikZ.
    // Nome derivato: se file_output finisce con ".dot" lo sostituiamo con
    // ".tikz.dot", altrimenti append.
    std::string tikz_out = file_output;
    const std::string ext = ".dot";
    if (tikz_out.size() >= ext.size() &&
        tikz_out.compare(tikz_out.size() - ext.size(), ext.size(), ext) == 0) {
        tikz_out.resize(tikz_out.size() - ext.size());
    }
    tikz_out += ".tikz.dot";
    salva_tikz_dot(tikz_out, circuito);

    // calcolo i ciclo fondamentali
    std::vector<std::list<int>> essential_cycles;
    find_essential_cycles_dfs(circuito, essential_cycles);

    for(const auto& cycle : essential_cycles) {
        for(const auto& node : cycle) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
    } 
}
