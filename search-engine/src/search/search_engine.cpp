#include "search_engine.hpp"
#include "../index/index_builder.hpp"
#include "../tokenizer/tokenizer.hpp"
#include "../tokenizer/html_strip.hpp"
#include "../stemmer/stemmer.hpp"
#include "boolean_query_parser.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <stdexcept>

#ifdef ENABLE_MONGODB
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/config/version.hpp>
#endif

SearchEngine::SearchEngine()
    : index_(1<<16) {}

bool SearchEngine::loadFromSampleFile(const std::string& path, std::string* err) {
    std::ifstream f(path);
    if (!f) {
        if (err) *err = "Cannot open sample file: " + path;
        return false;
    }
    documents_.clear();
    std::string line;
    int id = 0;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        size_t t1 = line.find('\t');
        size_t t2 = (t1 == std::string::npos) ? std::string::npos : line.find('\t', t1 + 1);
        if (t1 == std::string::npos || t2 == std::string::npos) continue;

        Document d;
        d.id = id++;
        d.url = line.substr(0, t1);
        d.crawled_at = line.substr(t1 + 1, t2 - (t1 + 1));
        d.html = line.substr(t2 + 1);
        documents_.push_back(std::move(d));
    }
    if (documents_.empty()) {
        if (err) *err = "No documents loaded from sample file (bad format?)";
        return false;
    }
    return true;
}

bool SearchEngine::loadFromMongo(const MongoConfig& cfg, std::string* err) {
#ifndef ENABLE_MONGODB
    if (err) *err = "MongoDB support is disabled. Rebuild with -DENABLE_MONGODB=ON and ensure mongocxx is installed.";
    return false;
#else
    static mongocxx::instance inst{};

    try {
        mongocxx::client client{mongocxx::uri{cfg.uri}};
        auto coll = client[cfg.db][cfg.collection];

        documents_.clear();
        int id = 0;

        auto elem_to_string = [](const bsoncxx::document::element& el) -> std::string {
#if defined(BSONCXX_VERSION_MAJOR) && ( (BSONCXX_VERSION_MAJOR > 3) || (BSONCXX_VERSION_MAJOR == 3 && BSONCXX_VERSION_MINOR >= 7) )
            auto sv = el.get_string().value;
            return std::string(sv.data(), sv.size());
#else
            return el.get_utf8().value.to_string();
#endif
        };

        auto cursor = coll.find({});
        for (auto&& doc : cursor) {
            Document d;
            d.id = id++;

            if (auto el = doc["text"]; el) {
                try { d.html = elem_to_string(el); } catch (...) {}
            }
            if (auto el = doc["url"]; el) {
                try { d.url = elem_to_string(el); } catch (...) {}
            }
            if (auto el = doc["crawled_at"]; el) {
                try { d.crawled_at = elem_to_string(el); } catch (...) {}
            }

            documents_.push_back(std::move(d));
        }

        if (documents_.empty()) {
            if (err) *err = "Mongo collection returned 0 documents.";
            return false;
        }
        return true;

    } catch (const std::exception& e) {
        if (err) *err = std::string("MongoDB error: ") + e.what();
        return false;
    }
#endif
}

void SearchEngine::buildIndex(bool enable_stemming) {
    index_.clear();
    universe_ = PostingList{};

    for (int i = 0; i < (int)documents_.size(); ++i) {
        documents_[i].id = i;
        universe_.addSortedUnique(i);
    }

    auto st = IndexBuilder::build(documents_, index_, enable_stemming);

    (void)st;
}

bool SearchEngine::exportZipfCSV(const std::string& path_csv, size_t max_terms, std::string* err) const {
    try {
        IndexBuilder::export_zipf_csv(index_, path_csv, max_terms);
        return true;
    } catch (const std::exception& e) {
        if (err) *err = e.what();
        return false;
    }
}


PostingList SearchEngine::evalOperandTerm(const std::string& term) const {
    const TermData* td = index_.find(term);
    if (!td) return PostingList{};
    return td->postings;
}

std::string SearchEngine::normalizeQueryPhrase(const std::string& phrase) {
    return HtmlStripper::normalize_for_phrase(phrase);
}

PostingList SearchEngine::evalOperandPhrase(const std::string& phrase) const {
    std::string norm_phrase = normalizeQueryPhrase(phrase);
    if (norm_phrase.empty()) return PostingList{};

    TokenizationStats dummy;
    std::vector<std::string> toks = Tokenizer::tokenize(norm_phrase, &dummy);
    if (toks.empty()) return PostingList{};

    for (auto& t : toks) t = Stemmer::stem(t);

    PostingList cand = evalOperandTerm(toks[0]);
    for (size_t i = 1; i < toks.size(); ++i) {
        PostingList next = evalOperandTerm(toks[i]);
        cand = PostingList::And(cand, next);
        if (cand.empty()) break;
    }
    if (cand.empty()) return cand;

    PostingList out;
    for (int doc_id : cand.docs()) {
        if (doc_id < 0 || doc_id >= (int)documents_.size()) continue;
        const auto& dn = documents_[doc_id].normalized;
        if (dn.find(norm_phrase) != std::string::npos) out.addSortedUnique(doc_id);
    }
    return out;
}

std::string SearchEngine::makeSnippet(const std::string& plain, size_t n) {
    if (plain.size() <= n) return plain;
    return plain.substr(0, n) + "...";
}

std::vector<SearchResult> SearchEngine::search(const std::string& query, size_t max_results) const {
    std::vector<SearchResult> results;
    if (documents_.empty()) return results;

    std::vector<QToken> rpn = BooleanQueryParser::toRPN(query);
    std::vector<PostingList> stack;
    stack.reserve(rpn.size());

    auto eval_term_token = [&](const std::string& raw) -> PostingList {
        TokenizationStats dummy;
        std::vector<std::string> toks = Tokenizer::tokenize(raw, &dummy);
        if (toks.empty()) return PostingList{};
        std::string t = Stemmer::stem(toks[0]);
        return evalOperandTerm(t);
    };

    for (const auto& t : rpn) {
        if (t.type == QTokType::TERM) {
            stack.push_back(eval_term_token(t.text));
        } else if (t.type == QTokType::PHRASE) {
            stack.push_back(evalOperandPhrase(t.text));
        } else if (t.type == QTokType::NOT) {
            if (stack.empty()) throw std::runtime_error("NOT operand missing");
            PostingList a = std::move(stack.back());
            stack.pop_back();
            stack.push_back(PostingList::Not(universe_, a));
        } else if (t.type == QTokType::AND || t.type == QTokType::OR) {
            if (stack.size() < 2) throw std::runtime_error("Binary operator operand missing");
            PostingList b = std::move(stack.back()); stack.pop_back();
            PostingList a = std::move(stack.back()); stack.pop_back();
            stack.push_back(t.type == QTokType::AND ? PostingList::And(a, b) : PostingList::Or(a, b));
        }
    }

    PostingList final_docs;
    if (!stack.empty()) final_docs = std::move(stack.back());

    size_t count = 0;
    for (int doc_id : final_docs.docs()) {
        if ((int)documents_.size() <= doc_id) continue;
        const auto& d = documents_[doc_id];
        results.push_back({d.url, makeSnippet(d.plain, 200)});
        if (++count >= max_results) break;
    }
    return results;
}
