/*
----------------------------------------------------------------
Copyright (c) 2026 Faris Muhammad. All rights reserved.

    Unauthorized copying, modification, distribution, or use of
    this file, via any medium, is strictly prohibited without
    prior written permission.

- farismuhd.dev
----------------------------------------------------------------
*/

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "farismuhd.h"

namespace fs = std::filesystem;

using Replacements = std::vector<std::pair<std::string, std::string>>;

static std::string git_read_file(const fs::path& path) {
    std::ifstream file(path);
    if (!file) return "";

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static std::string git_html_escape(const std::string& input) {
    std::string output;

    for (char c : input) {
        switch (c) {
            case '&':  output += "&amp;";  break;
            case '<':  output += "&lt;";   break;
            case '>':  output += "&gt;";   break;
            case '"':  output += "&quot;"; break;
            case '\'': output += "&#39;";  break;
            default:   output += c;        break;
        }
    }

    return output;
}

static std::string git_url_escape(const std::string& input) {
    static const char hex[] = "0123456789abcdef";
    std::string output;

    for (unsigned char c : input) {
        bool safe = (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') ||
                    c == '-' || c == '_' || c == '.' || c == '/' || c == '~';

        if (safe) {
            output += static_cast<char>(c);
        } else {
            output += '%';
            output += hex[c >> 4];
            output += hex[c & 15];
        }
    }

    return output;
}

static void git_replace_all(std::string& text, const std::string& from, const std::string& to) {
    std::size_t pos = 0;

    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static std::string git_render_template(const fs::path& path, const Replacements& values) {
    std::string text = git_read_file(path);

    for (const auto& [from, to] : values)
        git_replace_all(text, from, to);

    return text;
}

static crow::response git_html_response(const std::string& body) {
    crow::response response(body);
    response.code = 200;
    response.set_header("Content-Type", "text/html; charset=utf-8");
    return response;
}

static bool git_safe_project(const std::string& project) {
    return !project.empty() &&
           project.find("..") == std::string::npos &&
           project.find('/') == std::string::npos &&
           project.find('\\') == std::string::npos;
}

static bool git_safe_relative_path(const std::string& path) {
    if (path.empty()) return true;

    fs::path relative(path);
    if (relative.is_absolute()) return false;

    for (const fs::path& part : relative)
        if (part == "..") return false;

    return true;
}

static fs::path git_find_readme(const fs::path& directory) {
    static const fs::path names[] = {
        "README.md", "README.MD", "readme.md", "README", "readme", "README.txt"
    };

    for (const fs::path& name : names) {
        fs::path candidate = directory / name;
        if (fs::is_regular_file(candidate)) return candidate;
    }

    return {};
}

static std::string git_project_name(const std::string& project) {
    if (project.size() > 4 && project.compare(project.size() - 4, 4, ".git") == 0)
        return project.substr(0, project.size() - 4);

    return project;
}

static std::string git_query_string(const crow::request& req) {
    std::size_t pos = req.raw_url.find('?');
    return pos == std::string::npos ? "" : req.raw_url.substr(pos + 1);
}

static std::string git_breadcrumb_item(
    const std::string& item_template,
    const std::string& href,
    const std::string& name,
    const std::string& separator = " / "
) {
    std::string item = item_template;
    git_replace_all(item, "{{HREF}}", href);
    git_replace_all(item, "{{NAME}}", git_html_escape(name));
    git_replace_all(item, "{{SEPARATOR}}", separator);
    return item;
}

static std::string git_build_breadcrumb(
    const std::string& project,
    const fs::path& relative,
    bool is_file
) {
    std::string item_template = git_read_file("git/templates/breadcrumb-item.html");
    std::string base = "/repo/" + git_url_escape(project);
    std::string result = git_breadcrumb_item(item_template, base, project);

    fs::path directory = is_file ? relative.parent_path() : relative;
    fs::path current;

    for (const fs::path& part : directory) {
        current /= part;
        result += git_breadcrumb_item(
            item_template,
            base + "/" + git_url_escape(current.generic_string()),
            part.string()
        );
    }

    if (is_file) {
        result += git_breadcrumb_item(
            item_template,
            base + "/" + git_url_escape(relative.generic_string()),
            relative.filename().string(),
            ""
        );
    }

    return result;
}

static std::string git_file_rows(
    const fs::path& directory,
    const std::string& project,
    const std::string& relative_directory
) {
    std::vector<fs::directory_entry> entries;

    for (const fs::directory_entry& entry : fs::directory_iterator(directory)) {
        std::string name = entry.path().filename().string();
        if (name.empty() || name[0] == '.') continue;
        entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(),
              [](const fs::directory_entry& a, const fs::directory_entry& b) {
                  if (a.is_directory() != b.is_directory()) return a.is_directory();
                  return a.path().filename().string() < b.path().filename().string();
              });

    std::string row_template = git_read_file("git/templates/file-row.html");
    std::string base = "/repo/" + git_url_escape(project);
    std::string rows;

    auto add_row = [&](const std::string& icon, const std::string& href,
                       const std::string& name, const std::string& type) {
        std::string row = row_template;
        git_replace_all(row, "{{ICON_CLASS}}", icon);
        git_replace_all(row, "{{HREF}}", href);
        git_replace_all(row, "{{NAME}}", name);
        git_replace_all(row, "{{TYPE}}", type);
        rows += row;
    };

    if (!relative_directory.empty()) {
        fs::path parent = fs::path(relative_directory).parent_path();
        if (parent == ".") parent.clear();

        std::string href = base;
        if (!parent.empty()) href += "/" + git_url_escape(parent.generic_string());

        add_row("folder-icon", href, "..", "Parent directory");
    }

    for (const fs::directory_entry& entry : entries) {
        std::string name = entry.path().filename().string();
        std::string relative_name = relative_directory.empty() ? name : relative_directory + "/" + name;
        bool is_dir = entry.is_directory();

        add_row(is_dir ? "folder-icon" : "document-icon",
                base + "/" + git_url_escape(relative_name),
                git_html_escape(name),
                is_dir ? "Directory" : "File");
    }

    return rows;
}

static crow::response git_project(const std::string& project, const std::string& requested) {
    if (!git_safe_project(project)) return serve_error(400);
    if (!git_safe_relative_path(requested)) return serve_error(403);

    fs::path project_root = fs::weakly_canonical("git/projects") / project;
    if (!fs::is_directory(project_root)) return serve_error(404);

    std::error_code ec;
    fs::path selected = fs::weakly_canonical(
        requested.empty() ? project_root : project_root / fs::path(requested), ec);
    if (ec) return serve_error(404);

    fs::path relative = fs::relative(selected, project_root, ec);
    if (ec ||
        relative.is_absolute() ||
        relative == ".." ||
        relative.generic_string().rfind("../", 0) == 0)
        return serve_error(403);

    std::string page = git_read_file("git/project.html");
    if (page.empty()) return serve_error(404);

    if (fs::is_directory(selected)) {
        fs::path relative_directory = relative.generic_string() == "." ? fs::path() : relative;
        std::string rows = git_file_rows(selected, project, relative_directory.generic_string());

        std::string readme_html;
        fs::path readme = git_find_readme(selected);

        if (!readme.empty()) {
            readme_html = git_render_template(
                "git/templates/readme.html",
                {{"{{README_CONTENT}}", git_html_escape(git_read_file(readme))}});
        }

        std::string view = git_render_template(
            "git/templates/directory.html",
            {{"{{FILE_ROWS}}", rows}, {"{{README_HTML}}", readme_html}});

        git_replace_all(page, "{{PROJECT_NAME}}", git_html_escape(project));
        git_replace_all(page, "{{BREADCRUMB}}", git_build_breadcrumb(project, relative_directory, false));
        git_replace_all(page, "{{VIEW_HTML}}", view);

        return git_html_response(page);
    }

    if (fs::is_regular_file(selected)) {
        std::ifstream file(selected, std::ios::binary);
        if (!file) return serve_error(404);

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

        if (content.find('\0') != std::string::npos) return serve_file(selected);

        std::string view = git_render_template(
            "git/templates/file.html",
            {{"{{FILENAME}}", git_html_escape(relative.filename().string())},
             {"{{FILE_CONTENT}}", git_html_escape(content)}});

        git_replace_all(page, "{{PROJECT_NAME}}", git_html_escape(project));
        git_replace_all(page, "{{BREADCRUMB}}", git_build_breadcrumb(project, relative, true));
        git_replace_all(page, "{{VIEW_HTML}}", view);

        return git_html_response(page);
    }

    return serve_error(404);
}

static crow::response redirect_to_github(
    const crow::request& req,
    const std::string& project,
    const std::string& endpoint
) {
    std::string clean_project = git_project_name(project);
    if (!git_safe_project(clean_project)) return serve_error(400);

    std::string target = "https://github.com/" GITHUB_USERNAME "/" + clean_project + ".git/" + endpoint;
    std::string query = git_query_string(req);
    if (!query.empty()) {
        target += "?" + query;
    }

    crow::response res(307);
    res.set_header("Location", target);
    return res;
}

void git_main(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/repo/<string>/info/refs")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req, const std::string& project) {
        return redirect_to_github(req, project, "info/refs");
    });

    CROW_ROUTE(app, "/repo/<string>/git-upload-pack")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, const std::string& project) {
        return redirect_to_github(req, project, "git-upload-pack");
    });

    CROW_ROUTE(app, "/repo/<string>/git-receive-pack")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, const std::string& project) {
        return redirect_to_github(req, project, "git-receive-pack");
    });

    CROW_ROUTE(app, "/repo/<path>")
    ([](const std::string& requested) {
        fs::path path(requested);
        if (path.empty()) return serve_error(404);

        std::string project = path.begin()->string();
        if (!git_safe_project(project)) return serve_error(400);

        fs::path remaining;

        bool first = true;
        for (const fs::path& part : path) {
            if (first) {
                first = false;
                continue;
            }

            remaining /= part;
        }

        return git_project(
            project,
            remaining.empty() ? "" : remaining.generic_string()
        );
    });

    CROW_ROUTE(app, "/repo")
    ([]() {
        fs::path index = fs::path("git") / "index.html";
        if (!fs::is_regular_file(index)) return serve_error(404);
        return serve_file(index);
    });
}
