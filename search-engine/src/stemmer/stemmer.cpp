#include "stemmer.hpp"
#include <cctype>

bool Stemmer::ends_with(const std::string& s, const char* suf) {
    size_t n = 0;
    while (suf[n]) ++n;
    if (s.size() < n) return false;
    for (size_t i = 0; i < n; ++i) {
        if (s[s.size() - n + i] != suf[i]) return false;
    }
    return true;
}

static bool has_vowel(const std::string& s) {
    for (char c : s) {
        if (c=='a'||c=='e'||c=='i'||c=='o'||c=='u') return true;
    }
    return false;
}

std::string Stemmer::stem(const std::string& token) {
    if (token.size() <= 2) return token;
    std::string s = token;

    if (ends_with(s, "ies") && s.size() > 4) { s.resize(s.size()-3); s += "y"; }
    else if (ends_with(s, "sses") && s.size() > 4) { s.resize(s.size()-2); } // sses -> ss
    else if (ends_with(s, "s") && !ends_with(s, "ss") && s.size() > 3) { s.resize(s.size()-1); }

    // -ing / -ed
    if (ends_with(s, "ing") && s.size() > 5) {
        std::string base = s.substr(0, s.size()-3);
        if (has_vowel(base)) s = base;
    } else if (ends_with(s, "ed") && s.size() > 4) {
        std::string base = s.substr(0, s.size()-2);
        if (has_vowel(base)) s = base;
    }

    // -ly
    if (ends_with(s, "ly") && s.size() > 4) s.resize(s.size()-2);

    // -tion / -sion
    if (ends_with(s, "tion") && s.size() > 6) { s.resize(s.size()-4); s += "t"; }
    else if (ends_with(s, "sion") && s.size() > 6) { s.resize(s.size()-4); s += "s"; }

    // final 'e'
    if (ends_with(s, "e") && s.size() > 4) s.resize(s.size()-1);

    return s;
}
