#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <crow.h>

#include "farismuhd.h"

namespace fs = std::filesystem;

static inline bool is_safe_subpath(const std::string& p) {
    return !p.empty() &&
           p.find("..") == std::string::npos &&
           p.find('/')  == std::string::npos &&
           p.find('\\') == std::string::npos;
}

static fs::path find_index(const fs::path& base) {
    fs::path preferred = base / "index.html";

    if (fs::is_regular_file(preferred))
        return preferred;

    fs::path found;

    if (!fs::is_directory(base))
        return found;

    for (const fs::directory_entry& entry : fs::directory_iterator(base)) {
        if (!entry.is_regular_file())
            continue;

        fs::path file = entry.path();
        if (file.stem().string() == "index" &&
            file.extension() != "") {

            if (found.empty() || file.filename().string() < found.filename().string())
                found = file;
        }
    }

    return found;
}

static fs::path resolve_base(const crow::request& req) {
    std::string host = req.get_header_value("Host");

    auto colon = host.find(':');
    if (colon != std::string::npos)
        host = host.substr(0, colon);

    std::transform(host.begin(), host.end(), host.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });

    const std::string base_domain = HOST;

    if (host == base_domain)
        return "root";

    const std::string suffix = "." + base_domain;
    if (host.size() > suffix.size() &&
        host.compare(host.size() - suffix.size(),
                     suffix.size(),
                     suffix) == 0) {
        std::string sub = host.substr(0, host.size() - suffix.size());

        if (sub == "www")
            return "root";

        for (unsigned char c : sub) {
            if (!std::isalnum(c) && c != '-' && c != '_')
                return "";
        }

        if (find_index(sub).empty())
            return "";

        return sub;
    }

    return "";
}

static inline std::string error_text(int code) {
    switch (code) {
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default:  return "Error";
    }
}

std::string mime_for(const fs::path& path) {
    std::string ext = path.extension().string();

    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });

    if (ext == ".html" || ext == ".htm")
        return "text/html; charset=utf-8";
    if (ext == ".css")
        return "text/css; charset=utf-8";
    if (ext == ".js" || ext == ".mjs")
        return "application/javascript; charset=utf-8";
    if (ext == ".json")
        return "application/json";
    if (ext == ".xml")
        return "application/xml";
    if (ext == ".pdf")
        return "application/pdf";
    if (ext == ".txt")
        return "text/plain; charset=utf-8";
    if (ext == ".csv")
        return "text/csv; charset=utf-8";
    if (ext == ".svg")
        return "image/svg+xml";
    if (ext == ".png")
        return "image/png";
    if (ext == ".jpg" || ext == ".jpeg")
        return "image/jpeg";
    if (ext == ".gif")
        return "image/gif";
    if (ext == ".webp")
        return "image/webp";
    if (ext == ".bmp")
        return "image/bmp";
    if (ext == ".ico")
        return "image/x-icon";
    if (ext == ".mp4")
        return "video/mp4";
    if (ext == ".webm")
        return "video/webm";
    if (ext == ".mp3")
        return "audio/mpeg";
    if (ext == ".wav")
        return "audio/wav";
    if (ext == ".ogg")
        return "application/ogg";
    if (ext == ".wasm")
        return "application/wasm";
    if (ext == ".zip")
        return "application/zip";
    if (ext == ".woff")
        return "font/woff";
    if (ext == ".woff2")
        return "font/woff2";
    if (ext == ".ttf")
        return "font/ttf";

    return "application/octet-stream";
}

crow::response serve_error(int code, const fs::path& base) {
    crow::response res;
    res.code = code;

    fs::path filename = std::to_string(code) + ".html";

    fs::path candidates[] = {
        base / filename,
        base / "pages" / filename,
        fs::path("root") / filename,
        fs::path("root") / "pages" / filename,
        filename
    };

    for (const fs::path& candidate : candidates) {
        if (fs::is_regular_file(candidate)) {
            res.set_static_file_info(candidate.string(), mime_for(candidate));
            res.code = code;
            return res;
        }
    }

    const std::string text = error_text(code);
    res.set_header("Content-Type", "text/plain; charset=utf-8");
    res.body = text;
    return res;
}

crow::response serve_file(const fs::path& path, const std::string& mime) {
    if (!fs::is_regular_file(path))
        return serve_error(404);

    crow::response res;
    res.set_static_file_info(
        path.string(),
        mime.empty() ? mime_for(path) : mime
    );
    res.code = 200;

    return res;
}

int main(int argc, char* argv[]) {
    if (argc > 0)
        fs::current_path(fs::weakly_canonical(argv[0]).parent_path());

    crow::SimpleApp app;

    git_main(app);

    CROW_ROUTE(app, "/")
    ([](const crow::request& req) {
        fs::path base = resolve_base(req);

        if (base.empty())
            return serve_error(404);

        fs::path index = find_index(base);

        if (index.empty())
            return serve_error(404, base);

        return serve_file(index);
    });

    CROW_ROUTE(app, "/css/<path>")
    ([](const crow::request& req, const std::string& css_path) {
        fs::path base = resolve_base(req);

        if (base.empty())
            return serve_error(404);

        if (!is_safe_subpath(css_path))
            return serve_error(400, base);

        return serve_file(base / "css" / css_path);
    });

    CROW_ROUTE(app, "/js/<path>")
    ([](const crow::request& req, const std::string& js_path) {
        fs::path base = resolve_base(req);

        if (base.empty())
            return serve_error(404);

        if (!is_safe_subpath(js_path))
            return serve_error(400, base);

        return serve_file(base / "js" / js_path);
    });

    CROW_ROUTE(app, "/assets/<path>")
    ([](const crow::request& req, const std::string& asset_path) {
        fs::path base = resolve_base(req);

        if (base.empty())
            return serve_error(404);

        if (!is_safe_subpath(asset_path))
            return serve_error(400, base);

        return serve_file(base / "assets" / asset_path);
    });

    CROW_ROUTE(app, "/favicon.ico")
    ([](const crow::request& req) {
        fs::path base = resolve_base(req);

        if (base.empty())
            return serve_error(404);

        fs::path sub_icon = base / "assets" / "favicon.ico";
        if (fs::is_regular_file(sub_icon))
            return serve_file(sub_icon);

        fs::path global_icon = fs::path("assets") / "favicon.ico";
        if (fs::is_regular_file(global_icon))
            return serve_file(global_icon);

        return serve_error(404, base);
    });

    CROW_ROUTE(app, "/robots.txt")
    ([](const crow::request& req) {
        fs::path base = resolve_base(req);

        if (base.empty())
            return serve_error(404);

        fs::path sub_robots = base / "robots.txt";
        if (fs::is_regular_file(sub_robots))
            return serve_file(sub_robots);

        crow::response res("User-agent: *\nAllow: /\n");
        res.set_header("Content-Type", "text/plain; charset=utf-8");
        return res;
    });

    CROW_ROUTE(app, "/sitemap.xml")
    ([](const crow::request& req) {
        fs::path base = resolve_base(req);

        if (base.empty())
            return serve_error(404);

        fs::path sitemap = base / "sitemap.xml";
        if (!fs::is_regular_file(sitemap))
            return serve_error(404, base);

        return serve_file(sitemap);
    });

    CROW_ROUTE(app, "/llms.txt")
    ([]() {
        fs::path llms = "llms.txt";

        if (!fs::is_regular_file(llms))
            return serve_error(404);

        return serve_file(llms);
    });

    CROW_ROUTE(app, "/<path>")
    ([](const crow::request& req,
        const std::string& requested) -> crow::response {
        fs::path base = resolve_base(req);

        if (base.empty())
            return serve_error(404);

        if (!is_safe_subpath(requested))
            return serve_error(400, base);

        fs::path literal_file = base / "pages" / requested;
        if (fs::is_regular_file(literal_file))
            return serve_file(literal_file);

        fs::path requested_path(requested);
        if (requested_path.extension().empty()) {
            fs::path html_file = base / "pages" / (requested + ".html");

            if (fs::is_regular_file(html_file))
                return serve_file(html_file);
        }

        return serve_error(404, base);
    });

    app.port(PORT).multithreaded().run();
    return 0;
}
