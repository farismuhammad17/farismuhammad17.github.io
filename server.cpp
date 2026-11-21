#include <crow.h>
#include <string>

#define PORT 8080

static crow::response serve_file(const std::string& path, const std::string& mime = "") {
    crow::response res;

    if (mime.empty()) {
        res.set_static_file_info(path);
    } else {
        res.set_static_file_info(path, mime);
    }

    if (res.code == 404) {
        res.code = 404;
        res.body = "Not Found";
    }

    return res;
}

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]() {
        return serve_file("index.html", "text/html; charset=utf-8");
    });

    CROW_ROUTE(app, "/styles.css")([]() {
        return serve_file("styles.css", "text/css; charset=utf-8");
    });

    CROW_ROUTE(app, "/main.js")([]() {
        return serve_file("main.js", "application/javascript; charset=utf-8");
    });

    CROW_ROUTE(app, "/robots.txt")([]() {
        return serve_file("robots.txt", "text/plain; charset=utf-8");
    });

    CROW_ROUTE(app, "/assets/<path>")([](const std::string& asset_path) {
        if (asset_path.empty() || asset_path.find("..") != std::string::npos) {
            return crow::response(400, "Bad Request");
        }
        return serve_file("assets/" + asset_path);
    });

    app.port(PORT).multithreaded().run();

    return 0;
}
