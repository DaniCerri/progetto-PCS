#pragma once
#include <list>

template <typename T>
class Stack {
    std::list<T> elements;

public:
    // costruttore di default
    Stack() = default;
    
    // costruttore di copia
    Stack(const Stack& other) = default;
    
    // aggiungi un elemento
    void put(const T& nuovo) {
        elements.push_back(nuovo);
    }
    
    // rimuovi un elemento (e restituiscilo)
    T get() {
        T result = elements.back();
        elements.pop_back();
        return result;
    }

    // verifica se la pila è vuota
    bool empty() const {
        return elements.empty();
    }
};




