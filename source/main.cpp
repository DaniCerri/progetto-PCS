#include "parser/parser.hpp"
#include "unidirected_graph/unidirected_graph.hpp"
#include "unidirected_graph/dot_serializer.hpp"
#include <iostream>
#include <fstream>
#include "cycles/dfs_based.hpp"
#include "solver/build_matrix.hpp"
#include <Eigen/Dense>
#include "solver/gradiente_coniugato.hpp"
#include "solver/calc_voltage.hpp"

void salva_dot(const std::string& nome_file, const UnidirectedGraph<int>& circuito) {
    std::ofstream file(nome_file);
    to_dot(circuito, file, "Circuito");
}

void salva_tikz_dot(const std::string& nome_file, const UnidirectedGraph<int>& circuito) {
    std::ofstream file(nome_file);
    to_tikz_dot(circuito, file, "Circuito");
}

static std::string strip_suffix(const std::string& path, const std::string& suffix) {
    if (path.size() >= suffix.size() &&
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return path.substr(0, path.size() - suffix.size());
    }
    return path;
}

// output di default: ../out/<nome input>.dot
static std::string default_output_path(const std::string& file_input) {
    std::string base = file_input;
    const size_t slash = base.find_last_of("/\\");
    if (slash != std::string::npos) base = base.substr(slash + 1);
    return "../out/" + strip_suffix(base, ".txt") + ".dot";
}

// sostituisce il ".dot" finale con new_ext (o appende se assente)
static std::string with_extension(const std::string& dot_path, const std::string& new_ext) {
    return strip_suffix(dot_path, ".dot") + new_ext;
}

int main (const int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Utilizzo: " << argv[0]
                  << " <file_input> [file_output] [dfs|depina] [-v]" << std::endl;
        return 1;
    }

    std::string file_input = argv[1];
    std::string file_output;                 // vuoto -> percorso di default
    CycleType method = CycleType::DFS;
    bool verbose = false;

    // argomenti opzionali posizionali liberi: dfs/depina, -v, altrimenti file output
    for (int a = 2; a < argc; ++a) {
        const std::string arg = argv[a];
        if (arg == "dfs") {
            method = CycleType::DFS;
        } else if (arg == "depina") {
            method = CycleType::DePina;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (file_output.empty()) {
            file_output = arg;
        } else {
            std::cerr << "Argomento non riconosciuto: " << arg << std::endl;
            return 1;
        }
    }
    if (file_output.empty())
        file_output = default_output_path(file_input);

    try {
        Parser parser;
        UnidirectedGraph<int> circuito;

        parser.pipeline(file_input, circuito);

        salva_dot(file_output, circuito);
        salva_tikz_dot(with_extension(file_output, ".tikz.dot"), circuito);

        Eigen::MatrixXd R;          // resistenze (m x m)
        Eigen::MatrixXd B;          // incidenza  (m x n)
        Eigen::VectorXd v;          // termine noto (n)
        std::vector<std::vector<int>> essential_cycles;
        std::vector<UnidirectedEdge<int>> resistor_branches;
        build_matrices(circuito, R, B, v, essential_cycles, resistor_branches, method);

        Eigen::VectorXd i(essential_cycles.size());
        i.setZero();   // x0 = 0 per il gradiente coniugato

        const unsigned int iters = gradiente_coniugato(
            B.transpose() * R * B,
            v,
            i,
            1e-10
        );

        // i dump diagnostici vanno su stderr solo con -v: stdout = sole tensioni (spec. sez. 7)
        if (verbose) {
            for (const auto& cycle : essential_cycles) {
                for (int node : cycle) std::cerr << node << " ";
                std::cerr << "\n";
            }
            std::cerr << "Matrice delle resistenze R:\n" << R << "\n";
            std::cerr << "Matrice di incidenza B:\n" << B << "\n";
            std::cerr << "Termine noto v:\n" << v << "\n";
            std::cerr << "Correnti di maglia i:\n" << i << "\n";
            std::cerr << "Numero di iterazioni: " << iters << std::endl;
        }

        Eigen::VectorXd V;
        calc_voltage(R, B, i, resistor_branches, V);

        // cicli su file (un ciclo per riga, nodi separati da spazio, senza chiusura)
        std::ofstream cycles_file(with_extension(file_output, ".cycles.txt"));
        for (const auto& cycle : essential_cycles) {
            if (cycle.empty()) continue;
            for (int node : cycle) {
                cycles_file << node << " ";
            }
            cycles_file << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Errore: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
