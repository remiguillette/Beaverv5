#pragma once

#include <atomic>
#include <string>

#include "core/app_manager.h"
#include "ui/http/http_utils.h"

class HttpServerApp {
public:
    explicit HttpServerApp(AppManager& manager, int port = 5000);
    ~HttpServerApp();

    int run();
    void stop();

private:
    void handle_request(int client_socket);
    std::string read_file(const std::string& filepath) const;
    bool setup_socket();

    AppManager& manager_;
    int port_;
    int server_socket_;
    std::atomic<bool> running_;

    static void handle_signal(int signal_number);
    static HttpServerApp* active_instance_;
};
