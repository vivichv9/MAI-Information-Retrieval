#pragma once
#include <string>
#include <vector>

enum class QTokType { TERM, PHRASE, AND, OR, NOT, LPAREN, RPAREN };

struct QToken {
    QTokType type;
    std::string text;
};

class BooleanQueryParser {
public:

    static std::vector<QToken> toRPN(const std::string& query);

private:
    static std::vector<QToken> tokenizeQuery(const std::string& query);
    static int precedence(QTokType t);
    static bool isRightAssociative(QTokType t);
};
