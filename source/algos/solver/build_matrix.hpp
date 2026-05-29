#pragma once
#include "unidirected_graph/unidirected_graph.hpp"
#include "cycles/dfs_based.hpp"
#include <Eigen/Dense>

template<typename T>
void build_matrices(
    UnidirectedGraph<T>& graph, // Grafo del circuito
    Eigen::MatrixXd& resistance_matrix, // Matrice delle resistenze calcolata nel parser
    std::vector<std::list<T>>& fundamental_cycles,  // lista di cicli fondamentali

    Eigen::VectorXd& voltage_vector_out,  // Vettore in cui si metteranno i voltaggi
    Eigen::MatrixXd& incidence_matrix_out,  // Matrice di incidenza B
) {
    // cerchiamo i cicli fondamentali
    // TODO: sarà da generalizzare per usare De Pina
    find_essential_cycles_dfs(graph, fundamental_cycles);
    std::vector temp_voltage(fundamental_cycles.size());
    
    int num_cycle = 0;
    for (auto cycle : fundamental_cycles) {
        
        std::cout << "--- Analisi Nuovo Ciclo ---\n";

        // 2. Inizializziamo i due iteratori sfalsati
        auto it_u = cycle.begin();
        auto it_v = std::next(cycle.begin());

        // 3. Scorriamo il ciclo fino alla fine
        while (it_v != cycle.end()) {
            T u = *it_u;
            T v = *it_v;

            // QUI FAI IL TUO CONTROLLO SUL LATO 
            std::cout << "Coppia di nodi (lato): (" << u << " -> " << v << ")\n";
            
            

            // Avanziamo entrambi
            ++it_u;
            ++it_v;
        }
    
        num_cycle ++;
    }
    
}