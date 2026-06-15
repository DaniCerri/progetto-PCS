#include "gradiente_coniugato.hpp"
#include <cmath>
#include <limits>

unsigned int gradiente_coniugato(
    const Eigen::MatrixXd& A,
    const Eigen::VectorXd& b,
    Eigen::VectorXd& x,
    const double r_tol
) {
    Eigen::VectorXd r = b - A * x;
    Eigen::VectorXd p = r;

    unsigned int k = 0;
    // CG su SPD converge in <= n passi; il cap (con margine) evita loop infiniti
    const unsigned int max_iter = 2u * static_cast<unsigned int>(b.size()) + 50u;

    while(r.norm() > r_tol && k < max_iter) {
        const double denom = (p.transpose() * A * p).value();
        // soglia relativa alla scala di p (denom ~ ||A||*||p||^2): niente confronto
        // esatto a 0, evita la divisione per quasi-zero che farebbe esplodere alpha/beta
        if (std::abs(denom) <= std::numeric_limits<double>::epsilon() * p.squaredNorm()) break;
        const double alpha_k = (p.transpose() * r).value() / denom;
        x += alpha_k * p;
        r = b - A * x;
        const double beta_k = (p.transpose() * A * r).value() / denom;
        p = r - beta_k * p;
        k++;
    }

    return k;
}
