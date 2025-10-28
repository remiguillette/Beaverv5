#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct WifiStatus {
    bool available = false;
    bool connected = false;
    std::string interface_name;
    std::string status_text;
};

struct WebSocketStatus {
    bool listening = false;
    std::string address;
    std::string last_message;
    double uptime_seconds = -1.0;
};

struct BatteryStatus {
    bool present = false;
    int percentage = -1;
    std::string state;
};

struct DebianStatus {
    double uptime_seconds = 0.0;
    std::string uptime_human;
    std::string boot_time_iso;
    double load_average[3] = {0.0, 0.0, 0.0};
};

struct NetworkStatus {
    std::vector<std::uint16_t> listening_ports;
};

struct SystemStatusSnapshot {
    WifiStatus wifi;
    WebSocketStatus websocket;
    BatteryStatus battery;
    DebianStatus debian;
    NetworkStatus network;
    std::string generated_at_iso;
};

SystemStatusSnapshot collect_system_status();
std::string system_status_to_json(const SystemStatusSnapshot& status);

