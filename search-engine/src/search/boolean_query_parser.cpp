#include "boolean_query_parser.hpp"
#include <cctype>
#include <stdexcept>

static inline bool is_space(unsigned char c) { return std::isspace(c) != 0; }

static std::string to_upper_ascii(std::string s) {
    for (char& ch : s) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return s;
}

std::vector<QToken> BooleanQueryParser::tokenizeQuery(const std::string& q) {
    std::vector<QToken> out;
    size_t i = 0;

    auto push_word_or_op = [&](const std::string& w) {
        std::string up = to_upper_ascii(w);
        if (up == "AND") out.push_back({QTokType::AND, ""});
        else if (up == "OR") out.push_back({QTokType::OR, ""});
        else if (up == "NOT") out.push_back({QTokType::NOT, ""});
        else out.push_back({QTokType::TERM, w});
    };

    while (i < q.size()) {
        unsigned char c = static_cast<unsigned char>(q[i]);
        if (is_space(c)) { ++i; continue; }

        if (q[i] == '(') { out.push_back({QTokType::LPAREN, ""}); ++i; continue; }
        if (q[i] == ')') { out.push_back({QTokType::RPAREN, ""}); ++i; continue; }

        if (q[i] == '"') {
            ++i;
            std::string phrase;
            while (i < q.size() && q[i] != '"') {
                phrase.push_back(q[i++]);
            }
            if (i < q.size() && q[i] == '"') ++i;
            out.push_back({QTokType::PHRASE, phrase});
            continue;
        }

        std::string w;
        while (i < q.size()) {
            char ch = q[i];
            if (ch == '(' || ch == ')' || ch == '"') break;
            if (is_space(static_cast<unsigned char>(ch))) break;
            w.push_back(ch);
            ++i;
        }
        if (!w.empty()) push_word_or_op(w);
    }
    return out;
}

int BooleanQueryParser::precedence(QTokType t) {
    switch (t) {
        case QTokType::NOT: return 3;
        case QTokType::AND: return 2;
        case QTokType::OR:  return 1;
        default: return 0;
    }
}

bool BooleanQueryParser::isRightAssociative(QTokType t) {
    return (t == QTokType::NOT);
}

std::vector<QToken> BooleanQueryParser::toRPN(const std::string& query) {
    auto toks = tokenizeQuery(query);
    std::vector<QToken> output;
    std::vector<QToken> ops;

    for (size_t i = 0; i < toks.size(); ++i) {
        QToken t = toks[i];
        switch (t.type) {
            case QTokType::TERM:
            case QTokType::PHRASE:
                output.push_back(t);
                break;

            case QTokType::AND:
            case QTokType::OR:
            case QTokType::NOT: {
                while (!ops.empty()) {
                    QTokType top = ops.back().type;
                    if (top == QTokType::LPAREN) break;
                    int p1 = precedence(t.type);
                    int p2 = precedence(top);
                    if (p2 > p1 || (p2 == p1 && !isRightAssociative(t.type))) {
                        output.push_back(ops.back());
                        ops.pop_back();
                    } else break;
                }
                ops.push_back(t);
                break;
            }

            case QTokType::LPAREN:
                ops.push_back(t);
                break;

            case QTokType::RPAREN:
                while (!ops.empty() && ops.back().type != QTokType::LPAREN) {
                    output.push_back(ops.back());
                    ops.pop_back();
                }
                if (ops.empty() || ops.back().type != QTokType::LPAREN) {
                    throw std::runtime_error("Mismatched parentheses in query");
                }
                ops.pop_back();
                break;
        }
    }

    while (!ops.empty()) {
        if (ops.back().type == QTokType::LPAREN || ops.back().type == QTokType::RPAREN)
            throw std::runtime_error("Mismatched parentheses in query");
        output.push_back(ops.back());
        ops.pop_back();
    }
    return output;
}
