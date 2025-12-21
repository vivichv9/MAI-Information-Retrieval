#include "web_server.hpp"
#include <sstream>

#include <httplib.h>

static std::string html_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

static std::string render_page(const std::string& q, const std::vector<SearchResult>& results) {
    std::ostringstream oss;
    oss << "<!doctype html><html><head><meta charset='utf-8'>"
        << "<title>Search Engine</title>"
        << "<style>"
        << "body{font-family:system-ui,Arial;max-width:900px;margin:40px auto;padding:0 16px;}"
        << "input{width:80%;padding:10px;font-size:16px;}"
        << "button{padding:10px 14px;font-size:16px;}"
        << ".res{margin-top:18px;padding:12px;border:1px solid #ddd;border-radius:8px;}"
        << ".url{font-size:14px;color:#0b57d0;word-break:break-all;}"
        << ".snip{margin-top:8px;color:#222;}"
        << "</style></head><body>";

    oss << "<h2>Boolean Search</h2>";
    oss << "<form method='GET' action='/search'>"
        << "<input name='q' value='" << html_escape(q) << "' placeholder='cat AND dog'/>"
        << "<button type='submit'>Search</button>"
        << "</form>";

    if (!q.empty()) {
        oss << "<p>Query: <b>" << html_escape(q) << "</b> | Found: <b>" << results.size() << "</b></p>";
        for (const auto& r : results) {
            oss << "<div class='res'>"
                << "<div class='url'><a href='" << html_escape(r.url) << "' target='_blank'>"
                << html_escape(r.url) << "</a></div>"
                << "<div class='snip'>" << html_escape(r.snippet) << "</div>"
                << "</div>";
        }
    }

    oss << "<hr><p style='color:#666'>Operators: AND OR NOT, parentheses (), phrases \"like this\"</p>";
    oss << "</body></html>";
    return oss.str();
}

int WebServer::run(SearchEngine& engine, int port) {
    httplib::Server svr;

    svr.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(render_page("", {}), "text/html; charset=utf-8");
    });

    svr.Get("/search", [&](const httplib::Request& req, httplib::Response& res) {
        std::string q;
        if (req.has_param("q")) q = req.get_param_value("q");
        std::vector<SearchResult> results;
        try {
            results = engine.search(q, 50);
            res.set_content(render_page(q, results), "text/html; charset=utf-8");
        } catch (const std::exception& e) {
            std::string msg = std::string("<pre>Error: ") + html_escape(e.what()) + "</pre>";
            res.status = 400;
            res.set_content(render_page(q, {}) + msg, "text/html; charset=utf-8");
        }
    });

    // listen
    return svr.listen("0.0.0.0", port) ? 0 : 1;
}
