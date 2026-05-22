#pragma once
#include <string>
#include "unidirected_graph.hpp"

class Parser {
    // std::string output;  // TODO 

    private: 
        std::string read_file(const std::string& file_path);
        void parse_file(const std::string& data, const std::string& del = " ");

    public:
        Parser() = default;
        ~Parser() = default;

        void pipeline(std::string& file_path, UnidirectedGraph<int>& graph_out);
};

class Component {
    std::string name;
    double value;

    public: 
        Component(const std::string& name_, double value_) : name(name_), value(value_) {}
        ~Component() = default;

        std::string get_name() const { return name; }
        double get_value() const { return value; }

        // capiamo se un componente è una resistenza in base alla prima lettera del nome
        bool is_resistor() const { return name[0] == 'R'; }
};