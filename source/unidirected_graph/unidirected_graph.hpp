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
    void add_edge(const T& u, const T& v) {
        UnidirectedEdge<T> e(u, v);
        for (const auto& x : edges)
            if (x == e) return;
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
            if (other.edge_number(e) == other.edges.size())
                result.add_edge(e.from(), e.to());
        }
        return result;
    }

    // serializza il grafo in formato DOT (GraphViz)
    void to_dot(std::ostream& os, const std::string& nome = "G") const {
        os << "graph " << nome << " {\n";
        // nodi isolati (nel caso non abbiano archi)
        for (const auto& v : vertices)
            os << "  " << v << ";\n";
        // archi
        for (const auto& e : all_edges())
            os << "  " << e.from() << " -- " << e.to() << ";\n";
        os << "}\n";
    }
};
