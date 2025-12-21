#include "search/search_engine.hpp"
#include "web/web_server.hpp"
#include "cli/cli.hpp"
#include <iostream>
#include <string>

struct Args {
    bool web = false;
    bool cli = false;
    bool stemming = true;
    int port = 8080;

    bool use_mongo = false;
    MongoConfig mongo;

    std::string sample_file = "data/sample.tsv";
    bool export_zipf = false;
    std::string zipf_path = "data/zipf.csv";
};

static void print_usage(const char* argv0) {
    std::cout
        << "Usage:\n"
        << "  " << argv0 << " --cli [--no-stem] [--sample path]\n"
        << "  " << argv0 << " --web --port 8080 [--no-stem] [--sample path]\n"
        << "  " << argv0 << " --mongo --mongo-uri URI --mongo-db DB --mongo-col COL [--cli|--web]\n"
        << "  " << argv0 << " --export-zipf [--zipf-path data/zipf.csv]\n\n"
        << "Examples:\n"
        << "  " << argv0 << " --cli\n"
        << "  " << argv0 << " --web --port 8080\n"
        << "  " << argv0 << " --mongo --mongo-uri mongodb://localhost:27017 --mongo-db mydb --mongo-col docs --cli\n\n";
}

static bool parse_args(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s == "--web") a.web = true;
        else if (s == "--cli") a.cli = true;
        else if (s == "--no-stem") a.stemming = false;
        else if (s == "--port" && i + 1 < argc) a.port = std::stoi(argv[++i]);
        else if (s == "--sample" && i + 1 < argc) a.sample_file = argv[++i];
        else if (s == "--mongo") a.use_mongo = true;
        else if (s == "--mongo-uri" && i + 1 < argc) a.mongo.uri = argv[++i];
        else if (s == "--mongo-db" && i + 1 < argc) a.mongo.db = argv[++i];
        else if (s == "--mongo-col" && i + 1 < argc) a.mongo.collection = argv[++i];
        else if (s == "--export-zipf") a.export_zipf = true;
        else if (s == "--zipf-path" && i + 1 < argc) a.zipf_path = argv[++i];
        else if (s == "--help" || s == "-h") { print_usage(argv[0]); return false; }
        else {
            std::cerr << "Unknown arg: " << s << "\n";
            print_usage(argv[0]);
            return false;
        }
    }
    if (!a.web && !a.cli) a.cli = true; // default
    return true;
}

int main(int argc, char** argv) {
    Args args;
    if (!parse_args(argc, argv, args)) return 1;

    SearchEngine engine;

    std::string err;
    bool ok = false;
    if (args.use_mongo) {
        ok = engine.loadFromMongo(args.mongo, &err);
    } else {
        ok = engine.loadFromSampleFile(args.sample_file, &err);
    }
    if (!ok) {
        std::cerr << "Load error: " << err << "\n";
        return 2;
    }

    engine.buildIndex(args.stemming);

    if (args.export_zipf) {
        if (!engine.exportZipfCSV(args.zipf_path, 0, &err)) {
            std::cerr << "Zipf export error: " << err << "\n";
            return 3;
        }
        std::cout << "Zipf CSV exported to: " << args.zipf_path << "\n";
    }

    if (args.web) {
        std::cout << "Starting web server on http://localhost:" << args.port << "\n";
        return WebServer::run(engine, args.port);
    }
    return CLI::run(engine);
}
