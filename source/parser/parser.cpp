#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "unidirected_graph.hpp"
#include <Eigen/Dense>

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

void Parser::parse_file(
    const std::string& data, 
    UnidirectedGraph<int>& graph_out, 
    Eigen::MatrixXd& resistance_matrix_out,
    const std::string& del
) {
    std::vector<double> resistance_vector;
    std::vector<double> voltage_vector;

    // Dividiamo la stringa in righe
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

        // Prendiamo i restanti 2 elementi come nodo1, nodo2
        int n1 = std::stoi(tokens[2]);
        int n2 = std::stoi(tokens[3]);

        // Costruiamo il componente: nome, valore, e nodo positivo (= primo nodo letto)
        // Il segno del generatore nel termine noto verra' gestito a valle dal solver,
        // confrontando positive_node con il verso di percorrenza della maglia.
        Component comp(tokens[0], std::stod(tokens[1]), n1);

        // TODO: direttamente qua possiamo fare la matrice diagonale con le resistenze man mano che le becchiamo
        if (comp.is_resistor()) {
            resistance_vector.push_back(comp.get_value());
        }

        // Aggiungiamo l'edge al grafo
        graph_out.add_edge(n1, n2, comp);
    }

    // Ora facciamo i vettori e matrici di Eigen
    resistance_matrix_out = Eigen::Map<Eigen::VectorXd>(resistance_vector.data(), resistance_vector.size()).asDiagonal();
}

void Parser::pipeline(std::string& file_path, UnidirectedGraph<int>& graph_out, Eigen::MatrixXd& resistance_matrix_out) {
    std::cout << "Inizio la pipeline" << std::endl;  
    
    std::string file_data = read_file(file_path);
    std::cout << "File letto" << std::endl; 
    
    std::cout << "Inizio parsing" << std::endl; 

    parse_file(file_data, graph_out, resistance_matrix_out, " ");
    std::cout << "File parsato" << std::endl; 
}

