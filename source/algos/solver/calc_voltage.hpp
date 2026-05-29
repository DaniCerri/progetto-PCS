#pragma once
#include "unidirected_graph/unidirected_graph.hpp"
#include <Eigen/Dense>
#include <iostream>
#include <vector>

// Calcola le tensioni sui resistori col Metodo delle Correnti di Maglia.
//   I_ram = B i   (corrente in ciascun ramo resistivo)
//   V     = R I_ram = R B i   (caduta di tensione, specifica pag. 5)
// resistor_branches[k] e' l'arco della riga k (stesso ordine di B/R):
// serve solo per etichettare il singolo componente in output.
template <typename T>
void calc_voltage(
    const Eigen::MatrixXd& R,
    const Eigen::MatrixXd& B,
    const Eigen::VectorXd& i,
    const std::vector<UnidirectedEdge<T>>& resistor_branches,
    Eigen::VectorXd& V)
{
    const Eigen::VectorXd branch_current = B * i;   // I_k per ogni ramo resistivo
    V = R * branch_current;                         // v_R = R B i

    for (size_t k = 0; k < resistor_branches.size(); ++k) {
        for (const auto& c : resistor_branches[k].get_components()) {
            if (!c.is_resistor()) continue;
            // tensione del singolo resistore: V = R_comp * I_ramo
            // (coincide con V(k) se il ramo ha un solo resistore)
            const double v_comp = c.get_value() * branch_current(k);
            std::cout << c.get_name() << ": V = " << v_comp
                      << " volts, I = " << branch_current(k) << " amps." << std::endl;
        }
    }
}
