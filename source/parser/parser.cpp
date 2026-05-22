#pragma once
#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>

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

std::vector<std::map<std::string, int>> Parser::parse_file(std::string& data, std::string del = " ") {
    std::vector<std::map<std::string, int>> parsed_data;

    // Dividiamo la stringa in righe

    // Controlliamo se la riga è vuota o solo con spazi per saltarla
    
    // Dividiamo la riga in base agli spazi
    
    // Controlliamo che ci siano rimasti 4 elementi distinti
    // Prendiamo il primo elemento come chiave (nome componente)
    // Prendiamo i restanti 3 elementi come valori (valore, nodo1, nodo2)
    // Aggiungiamo il map alla lista

    return parsed_data;
}

void Parser::pipeline(std::string& file_path, UnidirectedGraph<int>& graph_out) {
    std::cout << "Inizio la pipeline" << std::endl;    
    
}

