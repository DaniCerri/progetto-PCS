#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "component.hpp"

template <typename T>
class UnidirectedEdge {
    T source;
    T destination;
    Component component;

public:
    UnidirectedEdge() = default;

    // normalizza sempre from <= to
    UnidirectedEdge(const T& s, const T& d)
        : source(s < d ? s : d),
          destination(s < d ? d : s) {}

    UnidirectedEdge(const T& s, const T& d, const Component& c)
        : source(s < d ? s : d),
          destination(s < d ? d : s),
          component(c) {}

    const T& from() const { return source; }
    const T& to()   const { return destination; }

    // confronta solo gli estremi: ignora il componente
    bool operator==(const UnidirectedEdge& other) const {
        return source == other.source && destination == other.destination;
    }

    bool operator<(const UnidirectedEdge& other) const {
        if (source != other.source) return source < other.source;
        return destination < other.destination;
    }

    friend std::ostream& operator<<(std::ostream& os, const UnidirectedEdge& e) {
        return os << "(" << e.source << ", " << e.destination << ")";
    }

    void set_component(const Component& c) {
        component = c;
    }

    const Component& get_component() const {
        return component;
    }

    std::string edge_to_string() const {
        std::string res = "(" + std::to_string(source) + ", " + std::to_string(destination) + ") { ";
        res += component.get_name();
        res += " }";
        return res;
    }
};
