#include "index_builder.hpp"
#include "../tokenizer/html_strip.hpp"
#include "../tokenizer/tokenizer.hpp"
#include "../stemmer/stemmer.hpp"
#include <algorithm>
#include <fstream>

BuildStats IndexBuilder::build(std::vector<Document>& docs,
                               HashTable<TermData>& index,
                               bool enable_stemming) {
    BuildStats stats;

    std::vector<std::string> tokens;
    tokens.reserve(4096);

    for (auto& d : docs) {
        d.plain = HtmlStripper::extract_span_text(d.html);
        d.normalized = HtmlStripper::normalize_for_phrase(d.plain);

        tokens.clear();
        Tokenizer::tokenize_into(d.plain, tokens, &stats.tokenization);

        if (enable_stemming) {
            for (auto& t : tokens) t = Stemmer::stem(t);
        }

        for (const auto& t : tokens) {
            if (t.empty()) continue;
            TermData& td = index.getOrCreate(t);
            td.total_tf += 1;
        }

        std::sort(tokens.begin(), tokens.end());
        tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());

        for (const auto& t : tokens) {
            if (t.empty()) continue;
            TermData& td = index.getOrCreate(t);
            td.postings.addSortedUnique(d.id);
        }

        stats.docs_indexed += 1;
    }

    stats.unique_terms = index.size();
    return stats;
}

void IndexBuilder::export_zipf_csv(const HashTable<TermData>& index,
                                  const std::string& path_csv,
                                  size_t max_terms) {
    struct Row { std::string term; uint32_t tf; };
    std::vector<Row> rows;
    rows.reserve(index.size());

    index.forEach([&](const std::string& key, const TermData& td){
        rows.push_back({key, td.total_tf});
    });

    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b){
        return a.tf > b.tf;
    });

    std::ofstream f(path_csv);
    f << "rank,term,tf\n";
    size_t limit = (max_terms == 0) ? rows.size() : std::min(max_terms, rows.size());
    for (size_t i = 0; i < limit; ++i) {
        f << (i+1) << "," << rows[i].term << "," << rows[i].tf << "\n";
    }
}
