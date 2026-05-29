#include "parser/parser.hpp"
#include "unidirected_graph/unidirected_graph.hpp"
#include <iostream>
#include <fstream>
#include "cycles/dfs_based.hpp"
#include "solver/build_matrix.hpp"
#include <Eigen/Dense>
#include "solver/gradiente_coniugato.hpp"
#include "solver/calc_voltage.hpp"

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

    // assemblo le matrici del Metodo delle Correnti di Maglia
    Eigen::MatrixXd R;          // resistenze (m x m)
    Eigen::MatrixXd B;          // incidenza  (m x n)
    Eigen::VectorXd v;          // termine noto (n)
    std::vector<std::vector<UnidirectedEdge<int>>> essential_cycles;
    std::vector<UnidirectedEdge<int>> resistor_branches;   // riga i di B/R -> arco resistore
    build_matrices(circuito, R, B, v, essential_cycles, resistor_branches);

    for (const auto& cycle : essential_cycles) {
        for (const auto& edge : cycle) {
            std::cout << edge.edge_to_string() << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Matrice delle resistenze R:\n" << R << std::endl;
    std::cout << "Matrice di incidenza B:\n" << B << std::endl;
    std::cout << "Termine noto v:\n" << v << std::endl;

    Eigen::VectorXd i(essential_cycles.size());
    
    gradiente_coniugato(
        B.transpose() * R * B,  // Matrice dei coefficienti
        v,                      // Vettore dei termini noti
        i,                      // Vettore incognite
        1e-10                   // Tolleranza
    );

    std::cout << "Correnti di maglia i:\n" << i << std::endl;

    // tensioni sui resistori: V = R B i
    Eigen::VectorXd V;
    calc_voltage(R, B, i, resistor_branches, V);
    /*
    // salvo i cicli su file per la visualizzazione (un ciclo per riga,
    // nodi separati da spazio)
    std::string cycles_out = file_output;
    if (cycles_out.size() >= ext.size() &&
        cycles_out.compare(cycles_out.size() - ext.size(), ext.size(), ext) == 0) {
        cycles_out.resize(cycles_out.size() - ext.size());
    }
    cycles_out += ".cycles.txt";
    std::ofstream cycles_file(cycles_out);
    for (const auto& cycle : essential_cycles) {
        for (const auto& node : cycle) {
            cycles_file << node << " ";
            std::cout << node << " ";
        }
        cycles_file << "\n";
        std::cout << std::endl;
    } */
}
