#include "html_strip.hpp"
#include <cctype>
#include <string>

static inline bool ieq(char a, char b) {
    return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
}

static bool starts_with_ci(const std::string& s, size_t pos, const char* kw) {
    for (size_t i = 0; kw[i]; ++i) {
        if (pos + i >= s.size()) return false;
        if (!ieq(s[pos + i], kw[i])) return false;
    }
    return true;
}

static size_t find_ci(const std::string& s, size_t from, const char* needle) {
    const size_t nlen = std::char_traits<char>::length(needle);
    if (nlen == 0) return from;
    for (size_t i = from; i + nlen <= s.size(); ++i) {
        bool ok = true;
        for (size_t j = 0; j < nlen; ++j) {
            if (!ieq(s[i + j], needle[j])) { ok = false; break; }
        }
        if (ok) return i;
    }
    return std::string::npos;
}

void HtmlStripper::decode_entities_inplace(std::string& s) {
    std::string out;
    out.reserve(s.size());

    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') { out.push_back(s[i]); continue; }
        size_t semi = s.find(';', i + 1);
        if (semi == std::string::npos || semi - i > 10) { out.push_back(s[i]); continue; }

        std::string ent = s.substr(i, semi - i + 1);
        if (ent == "&amp;") out.push_back('&');
        else if (ent == "&lt;") out.push_back('<');
        else if (ent == "&gt;") out.push_back('>');
        else if (ent == "&quot;") out.push_back('"');
        else if (ent == "&#39;") out.push_back('\'');
        else if (ent == "&nbsp;") out.push_back(' ');
        else {
            if (ent.size() >= 4 && ent[1] == '#' ) {
                int val = 0;
                bool ok = true;
                for (size_t k = 2; k + 1 < ent.size(); ++k) {
                    if (!std::isdigit(static_cast<unsigned char>(ent[k]))) { ok = false; break; }
                    val = val * 10 + (ent[k] - '0');
                    if (val > 0x10FFFF) { ok = false; break; }
                }
                if (ok && val > 0 && val <= 255) out.push_back(static_cast<char>(val));
                else out.append(ent);
            } else {
                out.append(ent);
            }
        }
        i = semi;
    }
    s.swap(out);
}

std::string HtmlStripper::strip(const std::string& html) {
    std::string out;
    out.reserve(html.size());

    bool in_tag = false;
    size_t i = 0;

    while (i < html.size()) {
        char c = html[i];

        if (!in_tag) {
            if (c == '<') {
                if (starts_with_ci(html, i, "<script")) {
                    size_t end = find_ci(html, i, "</script>");
                    if (end == std::string::npos) break;
                    i = end + 9;
                    continue;
                }
                if (starts_with_ci(html, i, "<style")) {
                    size_t end = find_ci(html, i, "</style>");
                    if (end == std::string::npos) break;
                    i = end + 8; // len("</style>")
                    continue;
                }
                in_tag = true;
                ++i;
                continue;
            } else {
                out.push_back(c);
                ++i;
                continue;
            }
        } else {
            if (c == '>') {
                in_tag = false;
                out.push_back(' ');
            }
            ++i;
        }
    }

    decode_entities_inplace(out);

    std::string norm;
    norm.reserve(out.size());
    bool ws = false;
    for (char ch : out) {
        unsigned char uc = static_cast<unsigned char>(ch);
        if (std::isspace(uc)) {
            if (!ws) norm.push_back(' ');
            ws = true;
        } else {
            norm.push_back(ch);
            ws = false;
        }
    }
    if (!norm.empty() && norm.front() == ' ') norm.erase(norm.begin());
    if (!norm.empty() && norm.back() == ' ') norm.pop_back();

    return norm;
}


std::string HtmlStripper::extract_span_text(const std::string& html) {
    std::string out;
    out.reserve(html.size() / 2);

    bool in_tag = false;
    int span_depth = 0;

    size_t i = 0;
    while (i < html.size()) {
        char ch = html[i];

        if (!in_tag) {
            if (ch == '<') {
                if (starts_with_ci(html, i, "<script")) {
                    size_t end = find_ci(html, i, "</script>");
                    if (end == std::string::npos) break;
                    i = end + 9;
                    continue;
                }
                if (starts_with_ci(html, i, "<style")) {
                    size_t end = find_ci(html, i, "</style>");
                    if (end == std::string::npos) break;
                    i = end + 8;
                    continue;
                }

                in_tag = true;
                size_t j = i + 1;
                if (j < html.size() && (html[j] == '!' || html[j] == '?')) {
                    size_t gt = html.find('>', j);
                    if (gt == std::string::npos) break;
                    i = gt + 1;
                    in_tag = false;
                    if (span_depth > 0) out.push_back(' ');
                    continue;
                }

                bool closing = false;
                if (j < html.size() && html[j] == '/') { closing = true; ++j; }

                while (j < html.size() && std::isspace(static_cast<unsigned char>(html[j]))) ++j;

                std::string tag;
                while (j < html.size()) {
                    unsigned char c2 = static_cast<unsigned char>(html[j]);
                    if (std::isalnum(c2)) {
                        tag.push_back(static_cast<char>(std::tolower(c2)));
                        ++j;
                    } else break;
                }

                if (tag == "span") {
                    if (closing) {
                        if (span_depth > 0) --span_depth;
                    } else {
                        ++span_depth;
                    }
                }

                // skip to end of tag
                size_t gt = html.find('>', j);
                if (gt == std::string::npos) break;
                i = gt + 1;
                in_tag = false;
                if (span_depth > 0) out.push_back(' ');
                continue;
            } else {
                if (span_depth > 0) out.push_back(ch);
                ++i;
                continue;
            }
        } else {
            if (ch == '>') in_tag = false;
            ++i;
        }
    }

    decode_entities_inplace(out);

    // normalize whitespace
    std::string norm;
    norm.reserve(out.size());
    bool ws = false;
    for (char c3 : out) {
        unsigned char uc = static_cast<unsigned char>(c3);
        if (std::isspace(uc)) {
            if (!ws) norm.push_back(' ');
            ws = true;
        } else {
            norm.push_back(c3);
            ws = false;
        }
    }
    if (!norm.empty() && norm.front() == ' ') norm.erase(norm.begin());
    if (!norm.empty() && norm.back() == ' ') norm.pop_back();
    return norm;
}

std::string HtmlStripper::normalize_for_phrase(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    bool ws = false;

    for (char ch : text) {
        unsigned char uc = static_cast<unsigned char>(ch);
        if (std::isalnum(uc)) {
            out.push_back(static_cast<char>(std::tolower(uc)));
            ws = false;
        } else {
            if (!ws) out.push_back(' ');
            ws = true;
        }
    }

    // trim/collapse
    if (!out.empty() && out.front() == ' ') out.erase(out.begin());
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}
