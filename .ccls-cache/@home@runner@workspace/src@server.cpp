#include "../include/http_utils.h"
#include "../include/html_generator.h"
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <signal.h>

const int PORT = 5000;
const int BUFFER_SIZE = 8192;
int server_socket = -1;

void signal_handler(int signum) {
    std::cout << "\nShutting down server..." << std::endl;
    if (server_socket != -1) {
        close(server_socket);
    }
    exit(0);
}

std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
    
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    
    std::string raw_request(buffer, bytes_read);
    HttpRequest request = parse_http_request(raw_request);
    
    std::cout << request.method << " " << request.path << std::endl;
    
    HttpResponse response;
    
    if (request.path == "/" || request.path == "/index.html") {
        std::vector<AppTile> apps = get_sample_apps();
        response.body = generate_menu_page_html(apps);
        response.headers["Content-Type"] = "text/html; charset=utf-8";
        response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
        
    } else if (request.path == "/api/menu") {
        std::vector<AppTile> apps = get_sample_apps();
        response.body = generate_menu_json(apps);
        response.headers["Content-Type"] = "application/json; charset=utf-8";
        response.headers["Access-Control-Allow-Origin"] = "*";
        
    } else if (request.path == "/css/styles.css") {
        response.body = read_file("public/css/styles.css");
        if (response.body.empty()) {
            response.status_code = 404;
            response.status_text = "Not Found";
            response.body = "CSS file not found";
        } else {
            response.headers["Content-Type"] = "text/css; charset=utf-8";
            response.headers["Cache-Control"] = "no-cache";
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

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }
    
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_socket);
        return 1;
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << PORT << std::endl;
        close(server_socket);
        return 1;
    }
    
    if (listen(server_socket, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket);
        return 1;
    }
    
    std::cout << "==================================================" << std::endl;
    std::cout << "BeaverKiosk C++ HTTP Server" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Server running on http://0.0.0.0:" << PORT << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);
        
        if (client_socket < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        handle_request(client_socket);
    }
    
    close(server_socket);
    return 0;
}
