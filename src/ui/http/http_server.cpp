#include "ui/http/http_server.h"

#include <arpa/inet.h>
#include <algorithm>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <cctype>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

HttpServerApp* HttpServerApp::active_instance_ = nullptr;

HttpServerApp::HttpServerApp(AppManager& manager, int port)
    : manager_(manager),
      port_(port),
      server_socket_(-1),
      running_(false) {}

HttpServerApp::~HttpServerApp() {
    stop();
    if (active_instance_ == this) {
        active_instance_ = nullptr;
    }
}

void HttpServerApp::handle_signal(int /*signal_number*/) {
    if (active_instance_ != nullptr) {
        std::cout << "\nSignal received. Shutting down HTTP server..." << std::endl;
        active_instance_->stop();
    }
}

bool HttpServerApp::setup_socket() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    if (listen(server_socket_, 16) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    return true;
}

int HttpServerApp::run() {
    active_instance_ = this;
    std::signal(SIGINT, HttpServerApp::handle_signal);
    std::signal(SIGTERM, HttpServerApp::handle_signal);

    if (!setup_socket()) {
        stop();
        return 1;
    }

    running_ = true;

    std::cout << "==================================================" << std::endl;
    std::cout << "BeaverKiosk C++ HTTP Server" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Server running on http://0.0.0.0:" << port_ << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "==================================================" << std::endl;

    while (running_) {
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);

        int client_socket = accept(server_socket_, reinterpret_cast<sockaddr*>(&client_address), &client_len);
        if (client_socket < 0) {
            if (!running_) {
                break;
            }
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }

        handle_request(client_socket);
    }

    stop();
    return 0;
}

void HttpServerApp::stop() {
    running_ = false;
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
}

std::string HttpServerApp::read_file(const std::string& filepath) const {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return {};
    }
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void HttpServerApp::handle_request(int client_socket) {
    constexpr std::size_t BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE] = {0};

    ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }

    std::string raw_request(buffer, static_cast<std::size_t>(bytes_read));
    HttpRequest request = parse_http_request(raw_request);

    std::cout << request.method << " " << request.path << std::endl;

    HttpResponse response;

    std::string path = request.path;
    std::map<std::string, std::string> query_parameters;
    const std::size_t query_start = path.find('?');
    if (query_start != std::string::npos) {
        query_parameters = parse_query_parameters(path.substr(query_start + 1));
        path = path.substr(0, query_start);
    }

    if (path.empty()) {
        path = "/";
    }

    Language language = manager_.get_default_language();
    auto lang_it = query_parameters.find("lang");
    if (lang_it != query_parameters.end()) {
        std::string value = lang_it->second;
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (value == "en") {
            language = Language::English;
        } else if (value == "fr") {
            language = Language::French;
        }
    }

    if (path == "/" || path == "/index.html") {
        response.body = manager_.to_html(language);
        response.headers["Content-Type"] = "text/html; charset=utf-8";
        response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
        response.headers["Content-Language"] = language == Language::French ? "fr" : "en";
    } else if (path == "/apps/beaverphone") {
        response.body = manager_.beaverphone_page_html(language);
        response.headers["Content-Type"] = "text/html; charset=utf-8";
        response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
        response.headers["Content-Language"] = language == Language::French ? "fr" : "en";
    } else if (path == "/api/menu") {
        response.body = manager_.to_json(language);
        response.headers["Content-Type"] = "application/json; charset=utf-8";
        response.headers["Access-Control-Allow-Origin"] = "*";
        response.headers["Content-Language"] = language == Language::French ? "fr" : "en";
    } else if (path == "/css/styles.css") {
        response.body = read_file("public/css/styles.css");
        if (response.body.empty()) {
            response.status_code = 404;
            response.status_text = "Not Found";
            response.body = "CSS file not found";
        } else {
            response.headers["Content-Type"] = "text/css; charset=utf-8";
            response.headers["Cache-Control"] = "no-cache";
        }
    } else if (request.path.rfind("/icons/", 0) == 0) {
        std::string file_path = "public" + request.path;
        response.body = read_file(file_path);
        if (response.body.empty()) {
            response.status_code = 404;
            response.status_text = "Not Found";
            response.body = "Icon not found";
        } else {
            response.headers["Content-Type"] = "image/svg+xml";
            response.headers["Cache-Control"] = "public, max-age=86400";
        }
    } else {
        response.status_code = 404;
        response.status_text = "Not Found";
        response.body = "<html><body><h1>404 - Not Found</h1><p>The requested path was not found.</p></body></html>";
        response.headers["Content-Type"] = "text/html; charset=utf-8";
    }

    std::string response_str = build_http_response(response);
    send(client_socket, response_str.c_str(), response_str.length(), 0);
    close(client_socket);
}
