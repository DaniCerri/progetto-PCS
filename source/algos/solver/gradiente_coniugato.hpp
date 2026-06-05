#pragma once
#include <Eigen/Dense>
#include <iostream>

void gradiente_coniugato(
    const Eigen::MatrixXd& A,  // Matrice dei coefficienti
    const Eigen::VectorXd& b,  // Vettore dei termini noti
    Eigen::VectorXd& x,        // Vettore incognite
    const double r_tol         // Tolleranza
) {
    Eigen::VectorXd r = b - A * x;  // Residuo al passo 0
    Eigen::VectorXd p = r;          // Direzione di ricerca al passo 0

    unsigned int k = 0;
    // Per una matrice SPD (qui B^T R B) il CG converge in al piu' n passi; lascio
    // margine per gli errori di arrotondamento. Il cap evita loop infiniti se il
    // sistema non e' convergente.
    const unsigned int max_iter = 2u * static_cast<unsigned int>(b.size()) + 50u;

    while(r.norm() > r_tol && k < max_iter) {
        const double denom = (p.transpose() * A * p).value();
        if (denom == 0.0) break;   // direzione A-ortogonale: niente piu' progresso
        const double alpha_k = (p.transpose() * r).value() / denom;
        x += alpha_k * p;
        r = b - A * x;
        const double beta_k = (p.transpose() * A * r).value() / denom;
        p = r - beta_k * p;
        k++;
    }

    std::cout << "Numero di iterazioni: " << k << std::endl;
}
