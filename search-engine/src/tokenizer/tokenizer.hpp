#pragma once
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

struct TokenizationStats {
    uint64_t total_tokens = 0;
    uint64_t total_token_chars = 0;
    uint64_t bytes_processed = 0;
    uint64_t millis = 0;

    double avg_token_len() const {
        return total_tokens ? (double)total_token_chars / (double)total_tokens : 0.0;
    }
    double kb_per_sec() const {
        if (millis == 0) return 0.0;
        double sec = (double)millis / 1000.0;
        return ((double)bytes_processed / 1024.0) / sec;
    }
};

class Tokenizer {
public:
    static std::vector<std::string> tokenize(const std::string& plain, TokenizationStats* stats = nullptr);

    static void tokenize_into(const std::string& plain, std::vector<std::string>& out, TokenizationStats* stats = nullptr);
};
