#pragma once
#include <string>

class Component {
    public:
        enum class Type { Resistor, Generator };

    private:
        std::string name;
        double value;
        // terminale marcato positivo (convenzione di riferimento)
        int positive_node;
        Type type;

        // derivazione permissiva: la validazione del nome (R/V) e' a carico del
        // parser; gli archi-chiave sintetici (tipo mai letto) cadono su Generator
        static Type type_from_name(const std::string& n) {
            return (!n.empty() && n[0] == 'R') ? Type::Resistor : Type::Generator;
        }

    public:
        // componente vuoto: solo per gli archi-chiave di lookup (valore mai letto)
        Component() : name(""), value(0.0), positive_node(0), type(Type::Generator) {}
        Component(const std::string& name_, double value_, int positive_node_)
            : name(name_), value(value_), positive_node(positive_node_),
              type(type_from_name(name_)) {}
        ~Component() = default;

        std::string get_name() const { return name; }
        double get_value() const { return value; }
        int get_positive_node() const { return positive_node; }
        Type get_type() const { return type; }

        bool is_resistor() const { return type == Type::Resistor; }
};
