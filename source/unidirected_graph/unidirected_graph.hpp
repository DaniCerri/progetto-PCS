#pragma once
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stdexcept>
#include "unidirected_edge.hpp"

template <typename T>
class UnidirectedGraph {
    // archi (sorgente di verita': ordine di inserimento, serve a all_edges() lex)
    std::vector<UnidirectedEdge<T>> edges;
    // nodi unici
    std::set<T> vertices;
    // lista di adiacenza: nodo -> indici degli archi incidenti in `edges`.
    // Rende vicinato/incidenza O(deg) invece di O(m) (scansione di tutti gli archi),
    // e il controllo duplicati in add_edge O(deg). Mantenuta in sincrono da add_edge;
    // operator- e graph_visit ricostruiscono il grafo via add_edge -> adj coerente.
    std::map<T, std::vector<size_t>> adj;

public:
    // costruttore di default
    UnidirectedGraph() = default;

    // costruttore di copia
    UnidirectedGraph(const UnidirectedGraph& other) = default;

    // aggiunge un arco al grafo senza duplicati
    // non prevedo di poter aggiungere nodi senza archi, non creo quindi nessun metodo add.node()
    // ma nel caso in cui si provi ad aggiungere un arco i cui nodi non sono presenti nel set, allora
    // vengono aggiunti automaticamente
    // Modello a singolo componente per arco: due componenti sugli stessi nodi
    // (u,v) NON possono condividere lo stesso arco. operator== confronta solo gli
    // estremi, quindi un secondo add_edge sullo stesso (u,v) verrebbe scartato in
    // silenzio perdendo il componente -> lanciamo invece un errore esplicito.
    // Rami in parallelo o componenti in serie vanno modellati con nodi intermedi.
    void add_edge(const T& u, const T& v, const Component& component) {
        UnidirectedEdge<T> e(u, v, component);
        add_edge(e);
    }

    void add_edge(const UnidirectedEdge<T>& e) {
        // controllo duplicati solo tra gli archi incidenti a un estremo (O(deg))
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

    // restituisce i vicini di un nodo (O(deg) tramite lista di adiacenza)
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

    // restituisce gli archi incidenti a un nodo (O(deg) tramite lista di adiacenza)
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

    // restituisce tutti gli archi unici
    std::set<UnidirectedEdge<T>> all_edges() const {
        return std::set<UnidirectedEdge<T>>(edges.begin(), edges.end());
    }

    // archi in ordine di inserimento (sorgente di verita').
    // Usato dalla serializzazione DOT, che deve preservare l'ordine di lettura
    // della netlist invece dell'ordine lessicografico di all_edges().
    const std::vector<UnidirectedEdge<T>>& get_edges() const {
        return edges;
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
                result.add_edge(e);
            }
        }
        return result;
    }
};
