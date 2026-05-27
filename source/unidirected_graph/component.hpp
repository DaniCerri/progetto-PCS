#pragma once
#include <string>

class Component {
    std::string name;
    double value;
    // nodo a cui e' connesso il terminale "+" (per generatori) oppure
    // il terminale marcato positivo (per resistori, convenzione di riferimento)
    int positive_node;

    public:
        Component(const std::string& name_, double value_, int positive_node_)
            : name(name_), value(value_), positive_node(positive_node_) {}
        ~Component() = default;

        std::string get_name() const { return name; }
        double get_value() const { return value; }
        int get_positive_node() const { return positive_node; }

        // capiamo se un componente è una resistenza in base alla prima lettera del nome
        bool is_resistor() const { return name[0] == 'R'; }
};