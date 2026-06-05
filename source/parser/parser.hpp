#pragma once
#include <string>
#include <vector>
#include "unidirected_graph.hpp"

class Parser {
    private:
        std::string read_file(const std::string& file_path);
        std::vector<std::string> split(const std::string& row, const std::string& del);
        void parse_file(
            const std::string& data,
            UnidirectedGraph<int>& graph_out,
            const std::string& del = " \t\r\f\v"
        );

    public:
        Parser() = default;
        ~Parser() = default;

        void pipeline(std::string& file_path, UnidirectedGraph<int>& graph_out);
};
