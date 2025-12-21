#pragma once
#include <string>
#include <vector>
#include "../document.hpp"
#include "../structures/hash_table.hpp"
#include "term_data.hpp"
#include "../tokenizer/tokenizer.hpp"

struct BuildStats {
    TokenizationStats tokenization;
    uint64_t docs_indexed = 0;
    uint64_t unique_terms = 0;
};

class IndexBuilder {
public:
    static BuildStats build(std::vector<Document>& docs,
                            HashTable<TermData>& index,
                            bool enable_stemming);

    static void export_zipf_csv(const HashTable<TermData>& index,
                                const std::string& path_csv,
                                size_t max_terms = 0);
};
