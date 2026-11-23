#pragma once

#include <filesystem>
#include <string>

#include <crow.h>

#define HOST "farismuhd.dev"
#define PORT 8080

namespace fs = std::filesystem;

std::string mime_for(const fs::path& path);
crow::response serve_file(
    const fs::path& path,
    const std::string& mime = ""
);
crow::response serve_error(
    int code,
    const fs::path& base = {}
);

void git_main(crow::SimpleApp& app);
