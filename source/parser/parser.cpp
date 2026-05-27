#pragma once
#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include "unidirected_graph.hpp"

// Metodo per leggere un file dato un percorso e ritornarlo come stringa
std::string Parser::read_file(const std::string& file_path) {
    // apro il file
    std::ifstream file_stream{file_path};
    
    // controllo se il file e' stato aperto correttamente
    if (!file_stream.is_open()) {
        throw std::runtime_error("Errore: Impossibile aprire il file " + file_path);
    }
    
    // leggo il contenuto del file in un buffer
    std::stringstream buffer;
    buffer << file_stream.rdbuf();
    
    // chiudo il file
    file_stream.close();
    
    // ritorno il contenuto del file
    return buffer.str();
}

void Parser::parse_file(const std::string& data, const std::string& del) {
    UnidirectedGraph<int> graph_out;

    // Dividiamo la stringa in righe
    // std::vector<std::string> rows;  // vettore che contiene le righe
    std::stringstream ss(data);  // Stream con il contenuto del file
    std::string row;  // riga temporanea

    while (std::getline(ss, row, '\n')) {
        // Controlliamo se la riga è vuota o solo con spazi per saltarla
        if (row.find_first_not_of(" \n\r\t\f\v") == std::string::npos) {
            continue;
        }

        // Dividiamo la riga in base al delimitatore (numero indefinito tra i valori)
        std::vector<std::string> tokens;
        size_t start = row.find_first_not_of(del);
        while (start != std::string::npos) {
            size_t end = row.find_first_of(del, start);
            tokens.push_back(row.substr(start, end - start));
            start = row.find_first_not_of(del, end);
        }

        // Controlliamo che ci siano rimasti 4 elementi distinti
        if (tokens.size() != 4) {
            throw std::runtime_error("Riga malformata: " + row);
        }

        // Costruiamo il Componente con il primo elemento (nome) e il secondo (valore)
        Component comp(tokens[0], std::stod(tokens[1]));

        // Prendiamo i restanti 2 elementi come nodo1, nodo2
        int n1 = std::stoi(tokens[2]);
        int n2 = std::stoi(tokens[3]);

        // Aggiungiamo l'edge al grafo TODO: aggiungere anche il componente
        graph_out.add_edge(n1, n2);
    }
}

void Parser::pipeline(std::string& file_path, UnidirectedGraph<int>& graph_out) {
    std::cout << "Inizio la pipeline" << std::endl;    
}

