#pragma once
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include "unidirected_edge.hpp"

template <typename T>
class UnidirectedGraph {
    // archi
    std::vector<UnidirectedEdge<T>> edges;
    // nodi unici
    std::set<T> vertices;

public:
    // costruttore di default
    UnidirectedGraph() = default;

    // costruttore di copia
    UnidirectedGraph(const UnidirectedGraph& other) = default;

    // aggiunge un arco al grafo senza duplicati
    // non prevedo di poter aggiungere nodi senza archi, non creo quindi nessun metodo add.node()
    // ma nel caso in cui si provi ad aggiungere un arco i cui nodi non sono presenti nel set, allora
    // vengono aggiunti automaticamente
    void add_edge(const T& u, const T& v, const Component& component) {
        UnidirectedEdge<T> e(u, v);
        // aggiungiamo i componenti al lato del grafo appena aggiunto
        e.add_component(component);
        for (auto& x : edges)
            // TODO: migliorare controllo che il componente non sia duplicato nel ramo
            if (x == e && x.get_components().begin()->get_name() != component.get_name()) {
                x.add_component(component);
                return;
            }
            else if (x == e) return;
        edges.push_back(e);
        vertices.insert(u);
        vertices.insert(v);
    }

    void add_edge(const T& u, const T& v, const std::vector<Component>& components) {
        UnidirectedEdge<T> e(u, v);
        // aggiungiamo i componenti al lato del grafo appena aggiunto
        e.add_components(components);
        for (auto& x : edges)
            // TODO: migliorare controllo che il componente non sia duplicato nel ramo
            if (x == e) {
                x.add_components(components);
                return;
            }
        edges.push_back(e);
        vertices.insert(u);
        vertices.insert(v);
    }

    // restituisce i vicini di un nodo
    std::list<T> neighbours(const T& node) const {
        std::list<T> result;
        for (const auto& e : edges) {
            if (e.from() == node && e.to() != node)
                result.push_back(e.to());
            else if (e.to() == node && e.from() != node)
                result.push_back(e.from());
        }
        return result;
    }

    // restituisce tutti gli archi unici
    std::set<UnidirectedEdge<T>> all_edges() const {
        return std::set<UnidirectedEdge<T>>(edges.begin(), edges.end());
    }

    // restituisce tutti i nodi
    std::set<T> all_nodes() const {
        return vertices;
    }

    // numerazione di un arco all'interno del grafo
    // se l'arco non e' presente, ritorna edges.size()
    size_t edge_number(const UnidirectedEdge<T>& e) const {
        for (size_t i = 0; i < edges.size(); i++)
            if (edges[i] == e) return i;
        return edges.size();
    }

    // arco corrispondente al numero passato
    UnidirectedEdge<T> edge_at(size_t number) const {
        // .at() esegue il controllo dei limiti e lancia std::out_of_range
        // se number non è valido (cioè >= edges.size())
        return edges.at(number);
    }

    // differenza: archi presenti in *this ma non in other
    UnidirectedGraph operator-(const UnidirectedGraph& other) const {
        UnidirectedGraph result;
        for (const auto& e : edges) {
            if (other.edge_number(e) == other.edges.size()) {
                for (const auto& c : e.get_components())
                    result.add_edge(e.from(), e.to(), c);
            }
        }
        return result;
    }

    // serializza il grafo in formato DOT "topologia-only" pensato per essere
    // consumato da uno script di conversione a CircuiTikZ.
    // Solo nodi del circuito (no nodi-componente intermedi) e archi con attributi
    // custom: comp, val, type ("R"|"V"), pos (nodo positivo).
    void to_tikz_dot(std::ostream& os, const std::string& nome = "G") const {
        os << "graph " << nome << " {\n";
        os << "  layout=neato;\n";
        os << "  overlap=false;\n";
        os << "  splines=false;\n";
        os << "  node [shape=point];\n";
        for (const auto& v : vertices)
            os << "  " << v << ";\n";
        for (const auto& e : edges) {
            for (const auto& c : e.get_components()) {
                os << "  " << e.from() << " -- " << e.to()
                   << " [comp=\"" << c.get_name() << "\""
                   << ", val=\"" << c.get_value() << "\""
                   << ", type=\"" << (c.is_resistor() ? "R" : "V") << "\""
                   << ", pos=\"" << c.get_positive_node() << "\""
                   << "];\n";
            }
        }
        os << "}\n";
    }

    // serializza il grafo in formato DOT (GraphViz) cercando di assomigliare a uno
    // schema circuitale: ogni componente diventa un nodo intermedio tra i due nodi del
    // circuito; layout neato con archi ortogonali.
    // Resistore  -> nodo rettangolare bianco con label "R1\n10Ω"
    // Generatore -> nodo circolare giallo con label "V1\n10V" e "+"/"-" sui terminali
    void to_dot(std::ostream& os, const std::string& nome = "G") const {
        os << "graph " << nome << " {\n";
        os << "  layout=neato;\n";
        os << "  splines=ortho;\n";
        os << "  overlap=false;\n";
        os << "  node [fontname=\"Helvetica\"];\n";
        os << "  edge [fontname=\"Helvetica\", penwidth=1.4, arrowhead=none];\n";

        // nodi del circuito: pallini neri etichettati con il numero del nodo
        os << "  // nodi del circuito\n";
        for (const auto& v : vertices) {
            os << "  n" << v
               << " [shape=circle, style=filled, fillcolor=black,"
               << " width=0.15, label=\"\", xlabel=\"" << v << "\"];\n";
        }

        // componenti: un nodo per ciascun componente di ciascun arco
        os << "  // componenti\n";
        for (const auto& e : edges) {
            for (const auto& c : e.get_components()) {
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
        }
        os << "}\n";
    }
};
