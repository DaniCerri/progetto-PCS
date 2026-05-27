#pragma once
#include <string>
#include "unidirected_graph.hpp"

class Parser {
    // std::string output;  // TODO 

    private: 
        std::string read_file(const std::string& file_path);
        void parse_file(const std::string& data, UnidirectedGraph<int>& graph_out, const std::string& del = " ");

    public:
        Parser() = default;
        ~Parser() = default;

        void pipeline(std::string& file_path, UnidirectedGraph<int>& graph_out);
};
