#pragma once
#include <string>

struct Document {
    int id = -1;
    std::string html;          // raw HTML
    std::string url;
    std::string crawled_at;

    std::string plain;         // HTML stripped (human-readable)
    std::string normalized;    // for phrase matching: lower + non-alnum->space + collapsed spaces
};

struct SearchResult {
    std::string url;
    std::string snippet;
};
