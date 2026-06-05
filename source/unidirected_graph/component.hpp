#pragma once
#include <string>

class Component {
    public:
        // Tipo del componente, derivato UNA volta alla costruzione dal nome.
        // La validazione dell'input (nome che inizia per 'R' o 'V') e' a carico del
        // parser, unico confine di fiducia: qui la derivazione e' permissiva
        // (non-'R' -> generatore) perche' archi-chiave/sintetici non hanno un nome
        // significativo e il loro tipo non viene mai letto.
        enum class Type { Resistor, Generator };

    private:
        std::string name;
        double value;
        // nodo a cui e' connesso il terminale "+" (per generatori) oppure
        // il terminale marcato positivo (per resistori, convenzione di riferimento)
        int positive_node;
        Type type;

        static Type type_from_name(const std::string& n) {
            return (!n.empty() && n[0] == 'R') ? Type::Resistor : Type::Generator;
        }

    public:
        // default: componente "vuoto", usato solo per archi-chiave costruiti per
        // il lookup (UnidirectedEdge(a,b)) in cui il componente non viene mai letto
        Component() : name(""), value(0.0), positive_node(0), type(Type::Generator) {}
        Component(const std::string& name_, double value_, int positive_node_)
            : name(name_), value(value_), positive_node(positive_node_),
              type(type_from_name(name_)) {}
        ~Component() = default;

        std::string get_name() const { return name; }
        double get_value() const { return value; }
        int get_positive_node() const { return positive_node; }
        Type get_type() const { return type; }

        // tipo derivato alla costruzione: niente piu' scansione della stringa a ogni
        // chiamata ne' dipendenza dal comportamento di name[0] su stringa vuota
        bool is_resistor() const { return type == Type::Resistor; }
};