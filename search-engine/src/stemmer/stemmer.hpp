#pragma once
#include <string>

class Stemmer {
public:
    static std::string stem(const std::string& token);

private:
    static bool ends_with(const std::string& s, const char* suf);
};
