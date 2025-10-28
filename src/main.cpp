#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/app_manager.h"
#include "ui/gtk/gtk_app.h"
#include "ui/http/http_server.h"

namespace {
void print_usage(const char* executable_name) {
    std::cout << "Usage: " << executable_name
              << " [--http|--gtk] [--port=NUMBER] [URL options]\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  --http           Run the built-in HTTP server (default).\n";
    std::cout << "  --gtk            Launch the GTK 4 desktop application.\n";
    std::cout << "  --port=NUMBER    Override the HTTP server port (default: 5000).\n";
    std::cout << "  --beaverdoc-local-url=URL     Override the BeaverDoc URL in kiosk mode.\n";
    std::cout << "  --beaverdoc-remote-url=URL    Override the BeaverDoc URL for the HTTP menu.\n";
    std::cout << "  --beaverdebian-local-url=URL  Override the BeaverDebian URL in kiosk mode.\n";
    std::cout << "  --beaverdebian-remote-url=URL Override the BeaverDebian URL for the HTTP menu.\n";
    std::cout << "  -h, --help       Show this message and exit.\n";
}
}

int main(int argc, char* argv[]) {
    AppManager manager;

    bool http_requested = false;
    bool gtk_requested = false;
    int port = 5000;
    std::string beaverdoc_local_url = "http://localhost:8000";
    std::string beaverdoc_remote_url = "http://192.168.1.76:8000";
    std::string beaverdebian_local_url = "http://localhost:9090/";
    std::string beaverdebian_remote_url = "http://192.168.1.76:9090/";

    std::vector<char*> gtk_args;
    gtk_args.reserve(static_cast<std::size_t>(argc) + 1);
    gtk_args.push_back(argv[0]);

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--http") {
            http_requested = true;
        } else if (arg == "--gtk") {
            gtk_requested = true;
        } else if (arg.rfind("--port=", 0) == 0) {
            try {
                port = std::stoi(arg.substr(7));
                if (port <= 0 || port > 65535) {
                    throw std::out_of_range("port");
                }
            } catch (const std::exception&) {
                std::cerr << "Invalid port supplied to --port. Please choose a value between 1 and 65535." << std::endl;
                return 1;
            }
        } else if (arg.rfind("--beaverdoc-local-url=", 0) == 0) {
            beaverdoc_local_url = arg.substr(std::string("--beaverdoc-local-url=").size());
        } else if (arg.rfind("--beaverdoc-remote-url=", 0) == 0) {
            beaverdoc_remote_url = arg.substr(std::string("--beaverdoc-remote-url=").size());
        } else if (arg.rfind("--beaverdebian-local-url=", 0) == 0) {
            beaverdebian_local_url = arg.substr(std::string("--beaverdebian-local-url=").size());
        } else if (arg.rfind("--beaverdebian-remote-url=", 0) == 0) {
            beaverdebian_remote_url = arg.substr(std::string("--beaverdebian-remote-url=").size());
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            gtk_args.push_back(argv[i]);
        }
    }

    if (http_requested && gtk_requested) {
        std::cerr << "Please choose either --http or --gtk." << std::endl;
        return 1;
    }

    if (!http_requested && !gtk_requested) {
        http_requested = true;
    }

    gtk_args.push_back(nullptr);
    int gtk_argc = static_cast<int>(gtk_args.size()) - 1;

    manager.set_app_routes("BeaverDoc", {beaverdoc_local_url, beaverdoc_remote_url});
    manager.set_app_routes("BeaverDebian", {beaverdebian_local_url, beaverdebian_remote_url});

    if (http_requested) {
        HttpServerApp server(manager, port);
        return server.run();
    }

    GtkApp app(manager);
    return app.run(gtk_argc, gtk_args.data());
}
