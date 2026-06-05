#pragma once
#include <ostream>
#include <string>
#include "unidirected_graph.hpp"

// Serializzazione del grafo-circuito in formato DOT.
// Tenuta fuori da UnidirectedGraph: la classe modella la topologia, non sa nulla
// di rendering ne' di convenzioni circuitali (R/V, terminale positivo). Queste
// funzioni libere leggono il grafo via accessori pubblici (all_nodes/get_edges).

// Formato DOT "topologia-only" pensato per essere consumato da uno script di
// conversione a CircuiTikZ. Solo nodi del circuito (no nodi-componente intermedi)
// e archi con attributi custom: comp, val, type ("R"|"V"), pos (nodo positivo).
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

// Formato DOT (GraphViz) che cerca di assomigliare a uno schema circuitale: ogni
// componente diventa un nodo intermedio tra i due nodi del circuito; layout neato
// con archi ortogonali.
// Resistore  -> nodo rettangolare bianco con label "R1\n10Ω"
// Generatore -> nodo circolare giallo con label "V1\n10V" e "+"/"-" sui terminali
template <typename T>
void to_dot(const UnidirectedGraph<T>& graph, std::ostream& os, const std::string& nome = "G") {
    os << "graph " << nome << " {\n";
    os << "  layout=neato;\n";
    os << "  splines=ortho;\n";
    os << "  overlap=false;\n";
    os << "  node [fontname=\"Helvetica\"];\n";
    os << "  edge [fontname=\"Helvetica\", penwidth=1.4, arrowhead=none];\n";

    // nodi del circuito: pallini neri etichettati con il numero del nodo
    os << "  // nodi del circuito\n";
    for (const auto& v : graph.all_nodes()) {
        os << "  n" << v
           << " [shape=circle, style=filled, fillcolor=black,"
           << " width=0.15, label=\"\", xlabel=\"" << v << "\"];\n";
    }

    // componenti: un nodo per ciascun componente di ciascun arco
    os << "  // componenti\n";
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
        // connessioni nodo_circuito -- componente -- nodo_circuito.
        // Per generatori marchiamo "+"/"-" sui due archi: il lato connesso
        // al positive_node riceve "+", l'altro "-".
        const int pos = c.get_positive_node();
        const T& other = (pos == e.from()) ? e.to() : e.from();
        if (c.is_resistor()) {
            // resistore: "+" solo sul terminale positivo (taillabel),
            // l'altro arco resta neutro
            os << "  n" << pos << " -- " << id
                << " [headlabel=\"+\", labelfontcolor=red,"
                << " labeldistance=1.5];\n";
            os << "  " << id << " -- n" << other << ";\n";
        } else {
            // generatore: "+" e "-" sui rispettivi archi
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
