#pragma once
#include <Eigen/Dense>

// Risolve il sistema lineare A x = b col metodo del Gradiente Coniugato.
// A deve essere simmetrica definita positiva (qui B^T R B). x e' input/output:
// vi si parte (x0) e vi si scrive la soluzione. r_tol e' la tolleranza sul residuo.
void gradiente_coniugato(
    const Eigen::MatrixXd& A,  // Matrice dei coefficienti
    const Eigen::VectorXd& b,  // Vettore dei termini noti
    Eigen::VectorXd& x,        // Vettore incognite
    const double r_tol         // Tolleranza
);
