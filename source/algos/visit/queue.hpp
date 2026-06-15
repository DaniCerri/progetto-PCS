#pragma once
#include <list>

template <typename T>
class Queue {
    std::list<T> elements;

public:
    Queue() = default;
    Queue(const Queue& other) = default;

    void put(const T& nuovo) {
        elements.push_back(nuovo);
    }

    T get() {
        T result = elements.front();
        elements.pop_front();
        return result;
    }

    bool empty() const {
        return elements.empty();
    }
};
