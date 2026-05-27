#pragma once
#include <list>

template <typename T>
class Queue {
    std::list<T> elements;

public:
    // costruttore di default
    Queue() = default;
    
    // costruttore di copia
    Queue(const Queue& other) = default;
    
    // aggiungi un elemento
    void put(const T& nuovo) {
        elements.push_back(nuovo);
    }
    
    // rimuovi un elemento (e restituiscilo)
    T get() {
        T result = elements.front();
        elements.pop_front();
        return result;
    }

    // verifica se la coda è vuota
    bool empty() const {
        return elements.empty();
    }
};




