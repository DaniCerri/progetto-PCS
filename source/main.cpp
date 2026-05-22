#include "parser/parser.hpp"
#include "unidirected_graph/unidirected_graph.hpp"
#include <iostream>

int main () {
    Parser parser;
    UnidirectedGraph<int> graph;

    // leggo la netlist
    std::string file_path = "input/Netlist.txt";
    parser.pipeline(file_path, graph);
}