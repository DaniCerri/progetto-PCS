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

void Parser::parse_file(std::string& data, std::string del = " ") {
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

        // Dividiamo la riga in base agli spazi
        
    }    
    // Controlliamo che ci siano rimasti 4 elementi distinti

    // Costruiamo il Componente con il primo elemento (nome) e il secondo elemento (valore)
    
    // Prendiamo i restanti 2 elementi come nodo1, nodo2

    // Aggiungiamo l'edge al grafo

    return parsed_data;
}

void Parser::pipeline(std::string& file_path, UnidirectedGraph<int>& graph_out) {
    std::cout << "Inizio la pipeline" << std::endl;    
    
}

