#pragma once
#include <list>

template <typename T>
class Stack {
    std::list<T> elements;

public:
    Stack() = default;
    Stack(const Stack& other) = default;

    void put(const T& nuovo) {
        elements.push_back(nuovo);
    }

    T get() {
        T result = elements.back();
        elements.pop_back();
        return result;
    }

    bool empty() const {
        return elements.empty();
    }
};
