#include "tokenizer.hpp"
#include <cctype>
#include <chrono>

static inline bool is_word_char(unsigned char c) {
    return std::isalnum(c) != 0;
}

static inline char to_lower_ascii(unsigned char c) {
    return static_cast<char>(std::tolower(c));
}

void Tokenizer::tokenize_into(const std::string& plain, std::vector<std::string>& out, TokenizationStats* stats) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();

    std::string cur;
    cur.reserve(32);

    for (size_t i = 0; i < plain.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(plain[i]);
        if (is_word_char(c)) {
            cur.push_back(to_lower_ascii(c));
        } else {
            if (!cur.empty()) {
                out.push_back(cur);
                if (stats) {
                    stats->total_tokens += 1;
                    stats->total_token_chars += cur.size();
                }
                cur.clear();
            }
        }
    }
    if (!cur.empty()) {
        out.push_back(cur);
        if (stats) {
            stats->total_tokens += 1;
            stats->total_token_chars += cur.size();
        }
    }

    if (stats) {
        stats->bytes_processed += plain.size();
        auto t1 = clock::now();
        stats->millis += (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    }
}

std::vector<std::string> Tokenizer::tokenize(const std::string& plain, TokenizationStats* stats) {
    std::vector<std::string> out;
    out.reserve(256);
    tokenize_into(plain, out, stats);
    return out;
}
