// Test della risoluzione del sistema lineare e del calcolo delle tensioni:
// valori golden dalla specifica + residuo del gradiente coniugato.
#include "test_harness.hpp"

// GOLDEN: netlist e risultati attesi della sez. 7 del PDF, per entrambi i metodi.
static void test_golden_sec7() {
    std::cout << "[solver] golden sez.7 (DFS + De Pina)\n";
    for (auto [m, mn] : std::vector<std::pair<CycleType, const char*>>{
             {CycleType::DFS, "DFS"}, {CycleType::DePina, "DePina"}}) {
        Solution s = solve("example_sec7.txt", m);
        const std::string t = mn;
        CHECK(s.resistors.size() == 5, t + ": 5 resistori");

        CHECK_NEAR(s.volt["R1"], 8.0, 1e-6, t + " V_R1");
        CHECK_NEAR(s.curr["R1"], 2.0, 1e-6, t + " I_R1");
        CHECK_NEAR(s.volt["R2"], 22.0, 1e-6, t + " V_R2");
        CHECK_NEAR(s.curr["R2"], 2.2, 1e-6, t + " I_R2");
        CHECK_NEAR(s.volt["R3"], -6.0, 1e-6, t + " V_R3");
        CHECK_NEAR(s.curr["R3"], -0.2, 1e-6, t + " I_R3");
        CHECK_NEAR(s.volt["R4"], -28.0, 1e-6, t + " V_R4");
        CHECK_NEAR(s.curr["R4"], -2.8, 1e-6, t + " I_R4");
        CHECK_NEAR(s.volt["R5"], 12.0, 1e-6, t + " V_R5");
        CHECK_NEAR(s.curr["R5"], 3.0, 1e-6, t + " I_R5");
    }
}

// GOLDEN: esempio a 2 maglie del PDF (sez. 4). I segni dipendono dalle maglie
// scelte, quindi confronto i MODULI: 10/11, 100/11, 120/11.
static void test_golden_two_mesh() {
    std::cout << "[solver] golden esempio 2-maglie (moduli)\n";
    for (auto m : {CycleType::DFS, CycleType::DePina}) {
        Solution s = solve("two_mesh.txt", m);
        CHECK(s.cycles.size() == 2, "2 maglie");
        CHECK_NEAR(std::abs(s.volt["R1"]), 10.0 / 11.0, 1e-4, "|V_R1|");
        CHECK_NEAR(std::abs(s.volt["R2"]), 100.0 / 11.0, 1e-4, "|V_R2|");
        CHECK_NEAR(std::abs(s.volt["R3"]), 120.0 / 11.0, 1e-4, "|V_R3|");
    }
}

// Maglia singola in serie V=12, R1=1, R2=2 -> I=4 A, V_Rk = Rk * I.
static void test_series_loop() {
    std::cout << "[solver] serie singola maglia\n";
    for (auto m : {CycleType::DFS, CycleType::DePina}) {
        Solution s = solve("series_loop.txt", m);
        CHECK(s.cycles.size() == 1, "1 maglia");
        CHECK_NEAR(std::abs(s.curr["R1"]), 4.0, 1e-6, "|I_R1| = 12/(1+2)");
        CHECK_NEAR(std::abs(s.curr["R2"]), 4.0, 1e-6, "|I_R2| = stessa corrente");
        CHECK_NEAR(std::abs(s.volt["R1"]), 4.0, 1e-6, "|V_R1| = 1*4");
        CHECK_NEAR(std::abs(s.volt["R2"]), 8.0, 1e-6, "|V_R2| = 2*4");
    }
}

// PROPRIETA': il gradiente coniugato risolve davvero B^T R B i = v.
static void test_residual() {
    std::cout << "[solver] residuo ||A i - v|| ~ 0\n";
    for (const char* nl : {"example_sec7.txt", "two_mesh.txt", "series_loop.txt"}) {
        for (auto m : {CycleType::DFS, CycleType::DePina}) {
            Solution s = solve(nl, m);
            const Eigen::MatrixXd A = s.B.transpose() * s.R * s.B;
            const double res = (A * s.i - s.v).norm();
            CHECK(res < 1e-6, std::string(nl) + ": residuo ~ 0");
        }
    }
}

int main() {
    test_golden_sec7();
    test_golden_two_mesh();
    test_series_loop();
    test_residual();
    return report("solver");
}
