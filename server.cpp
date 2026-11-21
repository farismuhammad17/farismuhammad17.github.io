#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <crow.h>

#define HOST "farismuhd.dev"
#define PORT 8080

namespace fs = std::filesystem;

static crow::response serve_file(const fs::path& path, const std::string& mime = "") {
    crow::response res;

    if (mime.empty()) {
        res.set_static_file_info(path.string());
    } else {
        res.set_static_file_info(path.string(), mime);
    }

    if (res.code == 404) {
        res.code = 404;
        res.body = "Not Found";
    }

    return res;
}

static bool is_safe_subpath(const std::string& p) {
    return !p.empty() && p.find("..") == std::string::npos && p.find('\\') == std::string::npos;
}

static fs::path resolve_base(const crow::request& req) {
    std::string host = req.get_header_value("Host");

    auto colon = host.find(':');
    if (colon != std::string::npos) {
        host = host.substr(0, colon);
    }

    std::transform(host.begin(), host.end(), host.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    const std::string base_domain = HOST;
    if (host == base_domain) {
        return "root";
    }

    const std::string suffix = "." + base_domain;
    if (host.size() > suffix.size() &&
        host.compare(host.size() - suffix.size(), suffix.size(), suffix) == 0) {
        std::string sub = host.substr(0, host.size() - suffix.size());
        if (sub == "www") return "root";

        for (unsigned char c : sub) {
            if (!std::isalnum(c) && c != '-' && c != '_') {
                return "";
            }
        }

        if (!fs::is_regular_file(fs::path(sub) / "index.html")) {
            return "";
        }
        return sub;
    }

    return "";
}

int main(int argc, char* argv[]) {
    if (argc > 0) {
        fs::current_path(fs::weakly_canonical(argv[0]).parent_path());
    }

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](const crow::request& req) {
        fs::path base = resolve_base(req);
        if (base.empty()) return crow::response(404, "Not Found");
        return serve_file(base / "index.html", "text/html; charset=utf-8");
    });

    CROW_ROUTE(app, "/css/<path>")([](const crow::request& req, const std::string& css_path) {
        fs::path base = resolve_base(req);
        if (base.empty()) return crow::response(404, "Not Found");
        if (!is_safe_subpath(css_path)) return crow::response(400, "Bad Request");
        return serve_file(base / "css" / css_path, "text/css; charset=utf-8");
    });

    CROW_ROUTE(app, "/js/<path>")([](const crow::request& req, const std::string& js_path) {
        fs::path base = resolve_base(req);
        if (base.empty()) return crow::response(404, "Not Found");
        if (!is_safe_subpath(js_path)) return crow::response(400, "Bad Request");
        return serve_file(base / "js" / js_path, "application/javascript; charset=utf-8");
    });

    CROW_ROUTE(app, "/assets/<path>")([](const crow::request& req, const std::string& asset_path) {
        fs::path base = resolve_base(req);
        if (base.empty()) return crow::response(404, "Not Found");
        if (!is_safe_subpath(asset_path)) return crow::response(400, "Bad Request");
        return serve_file(base / "assets" / asset_path);
    });

    CROW_ROUTE(app, "/favicon.ico")([](const crow::request& req) {
        fs::path base = resolve_base(req);
        if (base.empty()) return crow::response(404, "Not Found");

        fs::path sub_icon = base / "assets" / "favicon.ico";
        if (fs::exists(sub_icon)) {
            return serve_file(sub_icon, "image/x-icon");
        }
        return serve_file("favicon.ico", "image/x-icon");
    });

    CROW_ROUTE(app, "/robots.txt")([](const crow::request& req) {
        fs::path base = resolve_base(req);
        if (base.empty()) return crow::response(404, "Not Found");

        fs::path sub_robots = base / "robots.txt";
        if (fs::exists(sub_robots)) {
            return serve_file(sub_robots, "text/plain; charset=utf-8");
        }

        crow::response res("User-agent: *\nAllow: /\n");
        res.set_header("Content-Type", "text/plain; charset=utf-8");
        return res;
    });

    CROW_ROUTE(app, "/sitemap.xml")([](const crow::request& req) {
        fs::path base = resolve_base(req);
        if (base.empty()) return crow::response(404, "Not Found");
        return serve_file(base / "sitemap.xml", "application/xml; charset=utf-8");
    });

    CROW_ROUTE(app, "/llms.txt")([]() {
        return serve_file("llms.txt", "text/plain; charset=utf-8");
    });

    app.port(PORT).multithreaded().run();

    return 0;
}
