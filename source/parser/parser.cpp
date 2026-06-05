#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "unidirected_graph.hpp"

// Spezza una riga nei suoi token, usando come separatore qualsiasi
// carattere presente in 'del' (sequenze consecutive contano come uno).
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

// Metodo per leggere un file dato un percorso e ritornarlo come stringa
std::string Parser::read_file(const std::string& file_path) {
    // apro il file
    std::ifstream file_stream{file_path};
    
    // controllo se il file e' stato aperto correttamente
    if (!file_stream.is_open()) {
        throw std::runtime_error("Impossibile aprire il file " + file_path);
    }
    
    // leggo il contenuto del file in un buffer
    std::stringstream buffer;
    buffer << file_stream.rdbuf();
    
    // chiudo il file
    file_stream.close();
    
    // ritorno il contenuto del file
    return buffer.str();
}

void Parser::parse_file(
    const std::string& data,
    UnidirectedGraph<int>& graph_out,
    const std::string& del
) {
    // Dividiamo la stringa in righe
    std::stringstream ss(data);  // Stream con il contenuto del file
    std::string row;  // riga temporanea

    while (std::getline(ss, row, '\n')) {
        // Controlliamo se la riga è vuota o solo con spazi per saltarla
        if (row.find_first_not_of(" \n\r\t\f\v") == std::string::npos) {
            continue;
        }

        // Dividiamo la riga in base al delimitatore (numero indefinito tra i valori)
        std::vector<std::string> tokens = split(row, del);

        // Controlliamo che ci siano rimasti 4 elementi distinti
        if (tokens.size() != 4) {
            throw std::runtime_error("Riga malformata: " + row);
        }

        // Convertiamo valore e nodi, intercettando input non numerico
        // per dare un messaggio con il contesto della riga invece di
        // lasciar propagare std::invalid_argument/out_of_range grezzi.
        int n1, n2;
        double val;
        try {
            val = std::stod(tokens[1]);
            n1 = std::stoi(tokens[2]);
            n2 = std::stoi(tokens[3]);
        } catch (const std::exception&) {
            throw std::runtime_error("Valore non numerico nella riga: " + row);
        }

        // Costruiamo il componente: nome, valore, e nodo positivo (= primo nodo letto)
        // Il segno del generatore nel termine noto verra' gestito a valle dal solver,
        // confrontando positive_node con il verso di percorrenza della maglia.
        Component comp(tokens[0], val, n1);

        // Aggiungiamo l'edge al grafo
        graph_out.add_edge(n1, n2, comp);
    }
}

void Parser::pipeline(std::string& file_path, UnidirectedGraph<int>& graph_out) {
    std::string file_data = read_file(file_path);
    parse_file(file_data, graph_out, " \t\r\f\v");
}

