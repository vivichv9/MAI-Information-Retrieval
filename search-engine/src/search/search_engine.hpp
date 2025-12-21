#pragma once
#include <vector>
#include <string>
#include <optional>
#include "../document.hpp"
#include "../structures/hash_table.hpp"
#include "../index/term_data.hpp"
#include "../structures/posting_list.hpp"

struct MongoConfig {
    std::string uri = "mongodb://localhost:27017";
    std::string db = "search";
    std::string collection = "documents";
};

class SearchEngine {
public:
    SearchEngine();

    bool loadFromMongo(const MongoConfig& cfg, std::string* err = nullptr);
    bool loadFromSampleFile(const std::string& path, std::string* err = nullptr);

    void buildIndex(bool enable_stemming);

    std::vector<SearchResult> search(const std::string& query, size_t max_results = 50) const;

    bool exportZipfCSV(const std::string& path_csv, size_t max_terms = 0, std::string* err = nullptr) const;

    const std::vector<Document>& documents() const { return documents_; }

private:
    HashTable<TermData> index_;
    std::vector<Document> documents_;
    PostingList universe_;

    PostingList evalOperandTerm(const std::string& term) const;
    PostingList evalOperandPhrase(const std::string& phrase) const;
    static std::string makeSnippet(const std::string& plain, size_t n = 200);

    // Helpers for phrase:
    static std::string normalizeQueryPhrase(const std::string& phrase);
};
