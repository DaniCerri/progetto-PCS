#pragma once
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stdexcept>
#include "unidirected_edge.hpp"

template <typename T>
class UnidirectedGraph {
    // sorgente di verita': ordine di inserimento
    std::vector<UnidirectedEdge<T>> edges;
    std::set<T> vertices;
    // lista di adiacenza: nodo -> indici degli archi incidenti in `edges`.
    // Rende vicinato/incidenza e controllo duplicati O(deg); tenuta in sincrono da add_edge.
    std::map<T, std::vector<size_t>> adj;

public:
    UnidirectedGraph() = default;
    UnidirectedGraph(const UnidirectedGraph& other) = default;

    // nodi mancanti aggiunti in automatico (non esiste add_node).
    // Modello a singolo componente per arco: operator== confronta solo gli estremi,
    // quindi un secondo add_edge su (u,v) viene rifiutato con errore esplicito
    // invece di scartare in silenzio il componente. Rami paralleli/serie -> nodi intermedi.
    void add_edge(const T& u, const T& v, const Component& component) {
        UnidirectedEdge<T> e(u, v, component);
        add_edge(e);
    }

    void add_edge(const UnidirectedEdge<T>& e) {
        auto it = adj.find(e.from());
        if (it != adj.end())
            for (size_t idx : it->second)
                if (edges[idx] == e)
                    throw std::runtime_error(
                        "add_edge: arco duplicato - un solo componente per arco "
                        "(usa nodi intermedi per rami paralleli/serie)");

        const size_t idx = edges.size();
        edges.push_back(e);
        vertices.insert(e.from());
        vertices.insert(e.to());
        adj[e.from()].push_back(idx);
        if (e.to() != e.from())
            adj[e.to()].push_back(idx);
    }

    std::list<T> neighbours(const T& node) const {
        std::list<T> result;
        auto it = adj.find(node);
        if (it == adj.end()) return result;
        for (size_t idx : it->second) {
            const auto& e = edges[idx];
            if (e.from() == node && e.to() != node)
                result.push_back(e.to());
            else if (e.to() == node && e.from() != node)
                result.push_back(e.from());
        }
        return result;
    }

    std::vector<UnidirectedEdge<T>> incident_edges(const T& node) const {
        std::vector<UnidirectedEdge<T>> result;
        auto it = adj.find(node);
        if (it == adj.end()) return result;
        for (size_t idx : it->second) {
            const auto& e = edges[idx];
            if (e.from() != e.to())
                result.push_back(e);
        }
        return result;
    }

    // archi unici in ordine lessicografico
    std::set<UnidirectedEdge<T>> all_edges() const {
        return std::set<UnidirectedEdge<T>>(edges.begin(), edges.end());
    }

    // archi in ordine di inserimento (serve alla serializzazione DOT)
    const std::vector<UnidirectedEdge<T>>& get_edges() const {
        return edges;
    }

    std::set<T> all_nodes() const {
        return vertices;
    }

    // indice dell'arco, oppure edges.size() se assente
    size_t edge_number(const UnidirectedEdge<T>& e) const {
        for (size_t i = 0; i < edges.size(); i++)
            if (edges[i] == e) return i;
        return edges.size();
    }

    UnidirectedEdge<T> edge_at(size_t number) const {
        return edges.at(number);
    }

    // archi presenti in *this ma non in other
    UnidirectedGraph operator-(const UnidirectedGraph& other) const {
        UnidirectedGraph result;
        for (const auto& e : edges) {
            if (other.edge_number(e) == other.edges.size()) {
                result.add_edge(e);
            }
        }
        return result;
    }
};
