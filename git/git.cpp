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

static bool git_write_all(int fd, const std::string& data) {
    std::size_t written = 0;

    while (written < data.size()) {
        ssize_t result = write(fd, data.data() + written, data.size() - written);

        if (result > 0) written += static_cast<std::size_t>(result);
        else if (result < 0 && errno == EINTR) continue;
        else return false;
    }

    return true;
}

static std::string git_read_all(int fd) {
    std::string output;
    char buffer[65536];

    while (true) {
        ssize_t result = read(fd, buffer, sizeof(buffer));

        if (result > 0) output.append(buffer, static_cast<std::size_t>(result));
        else if (result < 0 && errno == EINTR) continue;
        else break;
    }

    return output;
}

static crow::response git_parse_cgi_output(const std::string& output) {
    std::size_t separator = output.find("\r\n\r\n");
    std::size_t separator_size = 4;

    if (separator == std::string::npos) {
        separator = output.find("\n\n");
        separator_size = 2;
    }

    if (separator == std::string::npos) return serve_error(502);

    std::string header_text = output.substr(0, separator);
    std::string body = output.substr(separator + separator_size);

    int status = 200;
    std::string content_type = "application/octet-stream";

    std::istringstream lines(header_text);
    std::string line;

    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        if (!value.empty() && value[0] == ' ') value.erase(0, 1);

        if (name == "Status") std::istringstream(value) >> status;
        else if (name == "Content-Type") content_type = value;
    }

    crow::response response(status, body);
    response.set_header("Content-Type", content_type);
    response.set_header("X-Content-Type-Options", "nosniff");
    return response;
}

static crow::response git_backend(
    const crow::request& req,
    const std::string& project,
    const std::string& endpoint
) {
    if (!git_safe_project(project)) return serve_error(400);

    std::string query = git_query_string(req);

    if (endpoint == "git-receive-pack" ||
        (endpoint == "info/refs" && query.find("service=git-receive-pack") != std::string::npos))
        return serve_error(403);

    fs::path projects_root = fs::weakly_canonical("git/projects");
    if (!fs::is_directory(projects_root / project)) return serve_error(404);

    int in_pipe[2];
    int out_pipe[2];

    if (pipe(in_pipe) != 0) return serve_error(500);

    if (pipe(out_pipe) != 0) {
        close(in_pipe[0]);
        close(in_pipe[1]);
        return serve_error(500);
    }

    pid_t child = fork();

    if (child < 0) {
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        return serve_error(500);
    }

    if (child == 0) {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);

        std::string method = endpoint == "info/refs" ? "GET" : "POST";
        std::string path_info = "/" + project + "/" + endpoint;
        std::string git_protocol = req.get_header_value("Git-Protocol");

        setenv("GIT_PROJECT_ROOT", projects_root.c_str(), 1);
        setenv("GIT_HTTP_EXPORT_ALL", "1", 1);
        setenv("REQUEST_METHOD", method.c_str(), 1);
        setenv("QUERY_STRING", query.c_str(), 1);
        setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
        setenv("PATH_INFO", path_info.c_str(), 1);
        setenv("REMOTE_USER", "", 1);
        setenv("REMOTE_ADDR", "", 1);
        setenv("CONTENT_TYPE", req.get_header_value("Content-Type").c_str(), 1);
        setenv("CONTENT_LENGTH", std::to_string(req.body.size()).c_str(), 1);

        if (!git_protocol.empty()) setenv("GIT_PROTOCOL", git_protocol.c_str(), 1);

        execl("/usr/lib/git-core/git-http-backend", "git-http-backend", nullptr);
        execl("/usr/libexec/git-core/git-http-backend", "git-http-backend", nullptr);
        execl("/usr/bin/git", "git", "http-backend", nullptr);

        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    git_write_all(in_pipe[1], req.body);
    close(in_pipe[1]);

    std::string cgi_output = git_read_all(out_pipe[0]);
    close(out_pipe[0]);

    waitpid(child, nullptr, 0);

    return git_parse_cgi_output(cgi_output);
}

void git_main(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/<string>/info/refs")
    ([](const crow::request& req, const std::string& project) {
        return git_backend(req, git_project_name(project), "info/refs");
    });

    CROW_ROUTE(app, "/<string>/git-upload-pack")
    ([](const crow::request& req, const std::string& project) {
        return git_backend(req, git_project_name(project), "git-upload-pack");
    });

    CROW_ROUTE(app, "/<string>/git-receive-pack")
    ([](const crow::request&, const std::string&) {
        return serve_error(403);
    });

    CROW_ROUTE(app, "/repo")
    ([]() {
        fs::path index = fs::path("git") / "index.html";
        if (!fs::is_regular_file(index)) return serve_error(404);
        return serve_file(index);
    });

    CROW_ROUTE(app, "/repo/<string>")
    ([](const std::string& project) {
        return git_project(project, "");
    });

    CROW_ROUTE(app, "/repo/<string>/<path>")
    ([](const std::string& project, const std::string& requested) {
        return git_project(project, requested);
    });
}
