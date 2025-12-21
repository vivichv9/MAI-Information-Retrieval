#pragma once
#include <string>

class HtmlStripper {
public:
    static std::string strip(const std::string& html);

    static std::string extract_span_text(const std::string& html);

    static std::string normalize_for_phrase(const std::string& text);

private:
    static void decode_entities_inplace(std::string& s);
};
