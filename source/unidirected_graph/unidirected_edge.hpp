#pragma once
#include <iostream>

template <typename T>
class UnidirectedEdge {
    T source;
    T destination;

public:
    // costruttore di default
    UnidirectedEdge() = default;

    // costruttore parametrico: garantisce sempre from <= to
    UnidirectedEdge(const T& s, const T& d)
        : source(s < d ? s : d),
          destination(s < d ? d : s) {}

    const T& from() const { return source; }
    const T& to()   const { return destination; }

    // operator==
    bool operator==(const UnidirectedEdge& other) const {
        return source == other.source && destination == other.destination;
    }

    // operator<: ordine lessicografico (source, destination)
    bool operator<(const UnidirectedEdge& other) const {
        if (source != other.source) return source < other.source;
        return destination < other.destination;
    }

    // operator<<
    friend std::ostream& operator<<(std::ostream& os, const UnidirectedEdge& e) {
        return os << "(" << e.source << ", " << e.destination << ")";
    }
};
