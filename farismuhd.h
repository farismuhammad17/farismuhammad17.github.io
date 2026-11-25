/*
----------------------------------------------------------------
Copyright (c) 2026 Faris Muhammad. All rights reserved.

    Unauthorized copying, modification, distribution, or use of
    this file, via any medium, is strictly prohibited without
    prior written permission.

- farismuhd.dev
----------------------------------------------------------------
*/

#pragma once

#include <filesystem>
#include <string>

#include <crow.h>

// PRODUCTION
// 0: Local testing / development
// 1: Production
#define PRODUCTION 1

#if PRODUCTION
    #define HOST "farismuhd.dev"
    #define PORT 8080
    #define SHOW_ERROR_SOURCE 0
#else
    #define HOST "localhost"
    #define PORT 8080
    #define SHOW_ERROR_SOURCE 1
#endif

#define GITHUB_USERNAME "farismuhammad17"

namespace fs = std::filesystem;

crow::response serve_error_impl(
    const std::string& where,
    int code,
    const fs::path& base = {}
);

#define serve_error(...) \
    serve_error_impl(std::string(__FILE__) + ":" + std::to_string(__LINE__), __VA_ARGS__)

std::string mime_for(const fs::path& path);
crow::response serve_file(
    const fs::path& path,
    const std::string& mime = ""
);

void git_main(crow::SimpleApp& app);
