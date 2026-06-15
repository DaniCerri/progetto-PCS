#pragma once
#include "unidirected_graph/unidirected_graph.hpp"
#include <Eigen/Dense>
#include <iostream>
#include <cmath>
#include <vector>

// Tensioni sui resistori col Metodo delle Correnti di Maglia: I_ram = B i, V = R B i.
template <typename T>
void calc_voltage(
    const Eigen::MatrixXd& R,
    const Eigen::MatrixXd& B,
    const Eigen::VectorXd& i,
    const std::vector<UnidirectedEdge<T>>& resistor_branches,
    Eigen::VectorXd& V)
{
    const Eigen::VectorXd branch_current = B * i;
    V = R * branch_current;

    // azzera il rumore numerico sotto il residuo del CG (e i -0.0) in stampa
    auto snap = [](double x) { return std::abs(x) < 1e-9 ? 0.0 : x; };

    for (size_t k = 0; k < resistor_branches.size(); ++k) {
        const Component& c = resistor_branches[k].get_component();
        if (!c.is_resistor()) continue;
        const double v_comp = snap(c.get_value() * branch_current(k));
        std::cout << c.get_name() << ": V = " << v_comp
                      << " volts, I = " << snap(branch_current(k)) << " amps." << std::endl;
    }
}
