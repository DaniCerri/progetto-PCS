#pragma once
#include <ostream>
#include <string>
#include "unidirected_graph.hpp"

// Serializzazione DOT del grafo-circuito, tenuta fuori da UnidirectedGraph (che
// modella solo la topologia). Legge il grafo via accessori pubblici.

// DOT "topologia-only" per la pipeline CircuiTikZ: solo nodi del circuito e archi
// con attributi custom (comp, val, type, pos).
template <typename T>
void to_tikz_dot(const UnidirectedGraph<T>& graph, std::ostream& os, const std::string& nome = "G") {
    os << "graph " << nome << " {\n";
    os << "  layout=neato;\n";
    os << "  overlap=false;\n";
    os << "  splines=false;\n";
    os << "  node [shape=point];\n";
    for (const auto& v : graph.all_nodes())
        os << "  " << v << ";\n";
    for (const auto& e : graph.get_edges()) {
        const auto& c = e.get_component();
        os << "  " << e.from() << " -- " << e.to()
            << " [comp=\"" << c.get_name() << "\""
            << ", val=\"" << c.get_value() << "\""
            << ", type=\"" << (c.is_resistor() ? "R" : "V") << "\""
            << ", pos=\"" << c.get_positive_node() << "\""
            << "];\n";
    }
    os << "}\n";
}

// DOT (GraphViz) in stile schema: ogni componente diventa un nodo intermedio tra
// i due nodi del circuito. Resistore -> rettangolo, generatore -> cerchio con +/-.
template <typename T>
void to_dot(const UnidirectedGraph<T>& graph, std::ostream& os, const std::string& nome = "G") {
    os << "graph " << nome << " {\n";
    os << "  layout=neato;\n";
    os << "  splines=ortho;\n";
    os << "  overlap=false;\n";
    os << "  node [fontname=\"Helvetica\"];\n";
    os << "  edge [fontname=\"Helvetica\", penwidth=1.4, arrowhead=none];\n";

    for (const auto& v : graph.all_nodes()) {
        os << "  n" << v
           << " [shape=circle, style=filled, fillcolor=black,"
           << " width=0.15, label=\"\", xlabel=\"" << v << "\"];\n";
    }

    for (const auto& e : graph.get_edges()) {
        const auto& c = e.get_component();
        const std::string id = "c_" + c.get_name();
        if (c.is_resistor()) {
            os << "  " << id
                << " [shape=rectangle, style=\"filled,rounded\","
                << " fillcolor=white, color=black,"
                << " label=\"" << c.get_name() << "\\n"
                << c.get_value() << "Ω\"];\n";
        } else {
            os << "  " << id
                << " [shape=circle, style=filled, fillcolor=lightyellow,"
                << " color=blue, fontcolor=blue,"
                << " label=\"" << c.get_name() << "\\n"
                << c.get_value() << "V\"];\n";
        }
        // "+" sul lato del positive_node, "-" sull'altro
        const int pos = c.get_positive_node();
        const T& other = (pos == e.from()) ? e.to() : e.from();
        if (c.is_resistor()) {
            os << "  n" << pos << " -- " << id
                << " [headlabel=\"+\", labelfontcolor=red,"
                << " labeldistance=1.5];\n";
            os << "  " << id << " -- n" << other << ";\n";
        } else {
            os << "  n" << pos << " -- " << id
                << " [headlabel=\"+\", labelfontcolor=red,"
                << " labeldistance=1.5];\n";
            os << "  " << id << " -- n" << other
                << " [taillabel=\"-\", labelfontcolor=red,"
                << " labeldistance=1.5];\n";
        }
    }
    os << "}\n";
}
