#pragma once
#include <Eigen/Dense>

// Gradiente Coniugato per A x = b, con A simmetrica definita positiva (qui B^T R B).
// x e' input/output (parte da x0, vi scrive la soluzione). Ritorna le iterazioni svolte.
unsigned int gradiente_coniugato(
    const Eigen::MatrixXd& A,
    const Eigen::VectorXd& b,
    Eigen::VectorXd& x,
    const double r_tol
);
