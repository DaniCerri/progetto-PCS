#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "unidirected_graph.hpp"

// separatore = qualsiasi carattere di 'del' (sequenze consecutive = un separatore)
std::vector<std::string> Parser::split(const std::string& row, const std::string& del) {
    std::vector<std::string> tokens;
    size_t start = row.find_first_not_of(del);
    while (start != std::string::npos) {
        size_t end = row.find_first_of(del, start);
        tokens.push_back(row.substr(start, end - start));
        start = row.find_first_not_of(del, end);
    }
    return tokens;
}

std::string Parser::read_file(const std::string& file_path) {
    std::ifstream file_stream{file_path};

    if (!file_stream.is_open()) {
        throw std::runtime_error("Impossibile aprire il file " + file_path);
    }

    std::stringstream buffer;
    buffer << file_stream.rdbuf();

    file_stream.close();
    return buffer.str();
}

void Parser::parse_file(
    const std::string& data,
    UnidirectedGraph<int>& graph_out,
    const std::string& del
) {
    std::stringstream ss(data);
    std::string row;

    while (std::getline(ss, row, '\n')) {
        // salto righe vuote o di soli spazi
        if (row.find_first_not_of(" \n\r\t\f\v") == std::string::npos) {
            continue;
        }

        std::vector<std::string> tokens = split(row, del);

        if (tokens.size() != 4) {
            throw std::runtime_error("Riga malformata: " + row);
        }

        // confine di fiducia sull'input: il tipo deve iniziare per R o V, cosi' la
        // classificazione a valle (Component::is_resistor) non si fonda su un nome arbitrario
        const char tipo = tokens[0].empty() ? '\0' : tokens[0][0];
        if (tipo != 'R' && tipo != 'V') {
            throw std::runtime_error(
                "Tipo componente non valido (atteso R o V) nella riga: " + row);
        }

        // conversione con contesto della riga nel messaggio d'errore
        int n1, n2;
        double val;
        try {
            val = std::stod(tokens[1]);
            n1 = std::stoi(tokens[2]);
            n2 = std::stoi(tokens[3]);
        } catch (const std::exception&) {
            throw std::runtime_error("Valore non numerico nella riga: " + row);
        }

        // nodo positivo = primo nodo letto (n1); il segno del generatore e' gestito dal solver
        Component comp(tokens[0], val, n1);

        graph_out.add_edge(n1, n2, comp);
    }
}

void Parser::pipeline(std::string& file_path, UnidirectedGraph<int>& graph_out) {
    std::string file_data = read_file(file_path);
    parse_file(file_data, graph_out, " \t\r\f\v");
}
