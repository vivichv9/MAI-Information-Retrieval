#include "cli.hpp"
#include <iostream>
#include <string>

int CLI::run(SearchEngine& engine) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (line == "exit") break;
        if (line.empty()) continue;

        try {
            auto results = engine.search(line, 50);
            std::cout << "Query: " << line << "\n";
            std::cout << "Found: " << results.size() << " documents\n";
            for (size_t i = 0; i < results.size(); ++i) {
                std::cout << (i + 1) << ". " << results[i].url << "\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    return 0;
}
