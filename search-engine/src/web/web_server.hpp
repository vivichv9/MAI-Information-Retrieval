#pragma once
#include <string>
#include "../search/search_engine.hpp"

class WebServer {
public:
    static int run(SearchEngine& engine, int port);
};
