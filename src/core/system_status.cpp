#include "core/system_status.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <string>

#if defined(__has_include)
#if __has_include(<sdbus-c++/sdbus-c++.h>)
#define BEAVER_HAVE_SDBUS 1
#include <sdbus-c++/sdbus-c++.h>
#endif
#endif

#if !defined(BEAVER_HAVE_SDBUS)
#define BEAVER_HAVE_SDBUS 0
#endif

namespace {

std::string trim(const std::string& input) {
    const auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (first >= last) {
        return std::string();
    }
    return std::string(first, last);
}

std::string read_file_trimmed(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return trim(buffer.str());
}

std::optional<double> parse_first_double(const std::string& text) {
    std::istringstream stream(text);
    double value = 0.0;
    if (!(stream >> value)) {
        return std::nullopt;
    }
    return value;
}

std::array<double, 3> parse_load_average(const std::string& text) {
    std::array<double, 3> load{0.0, 0.0, 0.0};
    std::istringstream stream(text);
    stream >> load[0] >> load[1] >> load[2];
    return load;
}

std::string format_uptime(double seconds) {
    if (!(seconds >= 0.0)) {
        return "";
    }

    auto total_seconds = static_cast<long long>(seconds);
    const long long days = total_seconds / 86400;
    total_seconds %= 86400;
    const long long hours = total_seconds / 3600;
    total_seconds %= 3600;
    const long long minutes = total_seconds / 60;
    const long long secs = total_seconds % 60;

    std::ostringstream formatted;
    if (days > 0) {
        formatted << days << "d ";
    }
    formatted << std::setfill('0') << std::setw(2) << hours << "h ";
    formatted << std::setfill('0') << std::setw(2) << minutes << "m ";
    formatted << std::setfill('0') << std::setw(2) << secs << "s";
    return formatted.str();
}

template <typename Duration>
std::string format_iso_timestamp(
    std::chrono::time_point<std::chrono::system_clock, Duration> time_point) {
    const auto system_time_point =
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(time_point);
    std::time_t time = std::chrono::system_clock::to_time_t(system_time_point);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream formatted;
    formatted << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return formatted.str();
}

std::vector<std::uint16_t> parse_tcp_table(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }

    std::set<std::uint16_t> ports;
    std::string line;
    // Skip header
    if (!std::getline(file, line)) {
        return {};
    }

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        std::string index;
        std::string local_address;
        std::string remote_address;
        std::string state;
        if (!(stream >> index >> local_address >> remote_address >> state)) {
            continue;
        }

        if (state != "0A") {  // Listening
            continue;
        }

        const std::size_t colon_position = local_address.find(':');
        if (colon_position == std::string::npos) {
            continue;
        }

        const std::string port_hex = local_address.substr(colon_position + 1);
        unsigned int port_value = 0;
        std::istringstream converter(port_hex);
        converter >> std::hex >> port_value;
        if (converter.fail()) {
            continue;
        }
        if (port_value == 0 || port_value > 65535) {
            continue;
        }
        ports.insert(static_cast<std::uint16_t>(port_value));
    }

    return {ports.begin(), ports.end()};
}

WifiStatus collect_wifi_status_proc() {
    WifiStatus status;
    const std::filesystem::path wireless_path("/proc/net/wireless");
    std::ifstream file(wireless_path);
    if (!file.is_open()) {
        return status;
    }

    std::string line;
    // Skip the two header lines
    std::getline(file, line);
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        const std::size_t colon_position = line.find(':');
        if (colon_position == std::string::npos) {
            continue;
        }
        std::string interface_name = trim(line.substr(0, colon_position));
        if (interface_name.empty()) {
            continue;
        }

        status.available = true;
        status.interface_name = interface_name;

        std::string payload = line.substr(colon_position + 1);
        std::istringstream stream(payload);
        int flags = 0;
        double link_quality = 0.0;
        stream >> std::hex >> flags;
        stream >> link_quality;

        bool connected = (link_quality > 0.0);

        if (!connected) {
            const std::filesystem::path operstate_path =
                std::filesystem::path("/sys/class/net") / interface_name / "operstate";
            const std::string operstate = read_file_trimmed(operstate_path);
            if (!operstate.empty()) {
                const std::string lowered = [&operstate]() {
                    std::string value = operstate;
                    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                        return static_cast<char>(std::tolower(ch));
                    });
                    return value;
                }();
                if (lowered == "up" || lowered == "unknown") {
                    connected = true;
                }
            }
        }

        status.connected = connected;
        status.status_text = status.connected ? "Connected" : "Not connected";
        break;
    }

    if (!status.available) {
        status.status_text = "Unavailable";
    }

    return status;
}

#if BEAVER_HAVE_SDBUS

std::string describe_nm_state(std::uint32_t state) {
    switch (state) {
        case 10:
            return "Asleep";
        case 20:
            return "Disconnected";
        case 30:
            return "Disconnecting";
        case 40:
            return "Connecting";
        case 50:
            return "Connected (local)";
        case 60:
            return "Connected (site)";
        case 70:
            return "Connected (global)";
        default:
            return "Unknown";
    }
}

WifiStatus collect_wifi_status_dbus() {
    WifiStatus status;
    status.interface_name = "NetworkManager";

    try {
        auto connection = sdbus::createSystemBusConnection();
        auto proxy =
            sdbus::createProxy(*connection, "org.freedesktop.NetworkManager",
                               "/org/freedesktop/NetworkManager");

        sdbus::Variant wireless_enabled_variant;
        proxy->callMethod("Get")
            .onInterface("org.freedesktop.DBus.Properties")
            .withArguments("org.freedesktop.NetworkManager", "WirelessEnabled")
            .storeResultsTo(wireless_enabled_variant);

        const bool wireless_enabled = wireless_enabled_variant.get<bool>();
        status.available = true;

        if (!wireless_enabled) {
            status.connected = false;
            status.status_text = "Disabled";
            return status;
        }

        sdbus::Variant state_variant;
        proxy->callMethod("Get")
            .onInterface("org.freedesktop.DBus.Properties")
            .withArguments("org.freedesktop.NetworkManager", "State")
            .storeResultsTo(state_variant);

        const std::uint32_t nm_state = state_variant.get<std::uint32_t>();
        status.connected = (nm_state >= 50U);
        status.status_text = describe_nm_state(nm_state);
    } catch (const std::exception& ex) {
        status.status_text = std::string("D-Bus error: ") + ex.what();
    }

    return status;
}

#endif  // BEAVER_HAVE_SDBUS

WifiStatus collect_wifi_status() {
    if (std::filesystem::exists("/proc/net/wireless")) {
        WifiStatus status = collect_wifi_status_proc();
        if (status.available) {
            return status;
        }
    }

#if BEAVER_HAVE_SDBUS
    {
        WifiStatus status = collect_wifi_status_dbus();
        if (status.available || !status.status_text.empty()) {
            return status;
        }
    }
#endif

    WifiStatus status;
    status.status_text = "Unavailable";
    return status;
}

BatteryStatus collect_battery_status() {
    BatteryStatus battery;
    namespace fs = std::filesystem;
    const fs::path power_supply_dir("/sys/class/power_supply");
    if (!fs::exists(power_supply_dir) || !fs::is_directory(power_supply_dir)) {
        battery.state = "Unavailable";
        return battery;
    }

    for (const auto& entry : fs::directory_iterator(power_supply_dir)) {
        if (!entry.is_directory()) {
            continue;
        }

        const fs::path& device_path = entry.path();
        const std::string type = read_file_trimmed(device_path / "type");
        if (type != "Battery") {
            continue;
        }

        battery.present = true;
        battery.state = read_file_trimmed(device_path / "status");
        if (battery.state.empty()) {
            battery.state = "Unknown";
        }

        const std::string capacity_text = read_file_trimmed(device_path / "capacity");
        if (!capacity_text.empty()) {
            try {
                battery.percentage = std::stoi(capacity_text);
            } catch (const std::exception&) {
                battery.percentage = -1;
            }
        }
        break;
    }

    if (!battery.present && battery.state.empty()) {
        battery.state = "Unavailable";
    }

    return battery;
}

std::array<double, 3> collect_load_average() {
    const std::string contents = read_file_trimmed("/proc/loadavg");
    if (contents.empty()) {
        return {0.0, 0.0, 0.0};
    }
    return parse_load_average(contents);
}

std::string json_escape(const std::string& input) {
    std::string escaped;
    escaped.reserve(input.size() + 16);
    for (char ch : input) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '\"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch));
                    escaped += oss.str();
                } else {
                    escaped += ch;
                }
        }
    }
    return escaped;
}

std::string format_double(double value, int precision = 2) {
    if (!std::isfinite(value)) {
        return "null";
    }
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::string optional_double(double value, int precision = 2) {
    if (!std::isfinite(value) || value < 0.0) {
        return "null";
    }
    return format_double(value, precision);
}

std::optional<std::uint16_t> parse_port_env(const char* name) {
    const char* value = std::getenv(name);
    if (!value || *value == '\0') {
        return std::nullopt;
    }
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value || (end && *end != '\0')) {
        return std::nullopt;
    }
    if (parsed <= 0 || parsed > 65535) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(parsed);
}

std::string build_websocket_address(std::uint16_t port) {
    if (const char* explicit_address = std::getenv("BEAVER_WS_ADDRESS"); explicit_address && *explicit_address) {
        return std::string(explicit_address);
    }

    std::string host = "localhost";
    if (const char* host_env = std::getenv("BEAVER_WS_HOST"); host_env && *host_env) {
        host = host_env;
    }

    std::ostringstream address;
    address << "ws://" << host << ':' << port;
    return address.str();
}

}  // namespace

SystemStatusSnapshot collect_system_status() {
    SystemStatusSnapshot snapshot;

    const auto uptime_contents = read_file_trimmed("/proc/uptime");
    if (const auto uptime_value = parse_first_double(uptime_contents)) {
        snapshot.debian.uptime_seconds = *uptime_value;
        snapshot.debian.uptime_human = format_uptime(*uptime_value);
        const auto boot_time_point = std::chrono::system_clock::now() -
                                     std::chrono::duration<double>(*uptime_value);
        snapshot.debian.boot_time_iso = format_iso_timestamp(boot_time_point);
    }

    const auto load_average = collect_load_average();
    snapshot.debian.load_average[0] = load_average[0];
    snapshot.debian.load_average[1] = load_average[1];
    snapshot.debian.load_average[2] = load_average[2];

    snapshot.network.listening_ports = parse_tcp_table("/proc/net/tcp");
    const auto tcp6_ports = parse_tcp_table("/proc/net/tcp6");
    snapshot.network.listening_ports.insert(snapshot.network.listening_ports.end(),
                                            tcp6_ports.begin(), tcp6_ports.end());
    std::sort(snapshot.network.listening_ports.begin(), snapshot.network.listening_ports.end());
    snapshot.network.listening_ports.erase(
        std::unique(snapshot.network.listening_ports.begin(), snapshot.network.listening_ports.end()),
        snapshot.network.listening_ports.end());

    snapshot.wifi = collect_wifi_status();
    snapshot.battery = collect_battery_status();

    snapshot.websocket.last_message.clear();
    snapshot.websocket.address.clear();
    snapshot.websocket.listening = false;
    snapshot.websocket.uptime_seconds = -1.0;

    const auto is_port_open = [&](std::uint16_t port) {
        return std::binary_search(snapshot.network.listening_ports.begin(),
                                  snapshot.network.listening_ports.end(), port);
    };

    if (const auto configured_port = parse_port_env("BEAVER_WS_PORT")) {
        snapshot.websocket.address = build_websocket_address(*configured_port);
        snapshot.websocket.listening = is_port_open(*configured_port);
    } else {
        constexpr std::uint16_t kLegacyWebsocketPort = 5001;
        if (is_port_open(kLegacyWebsocketPort)) {
            snapshot.websocket.address = build_websocket_address(kLegacyWebsocketPort);
            snapshot.websocket.listening = true;
        }
    }

    snapshot.generated_at_iso = format_iso_timestamp(std::chrono::system_clock::now());

    return snapshot;
}

std::string system_status_to_json(const SystemStatusSnapshot& status) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"generatedAt\": \"" << json_escape(status.generated_at_iso) << "\",\n";
    json << "  \"wifi\": {\n";
    json << "    \"available\": " << (status.wifi.available ? "true" : "false") << ",\n";
    json << "    \"connected\": " << (status.wifi.connected ? "true" : "false") << ",\n";
    json << "    \"interface\": \"" << json_escape(status.wifi.interface_name) << "\",\n";
    json << "    \"status\": \"" << json_escape(status.wifi.status_text) << "\"\n";
    json << "  },\n";
    json << "  \"websocket\": {\n";
    json << "    \"listening\": " << (status.websocket.listening ? "true" : "false") << ",\n";
    json << "    \"address\": \"" << json_escape(status.websocket.address) << "\",\n";
    json << "    \"lastMessage\": \"" << json_escape(status.websocket.last_message) << "\",\n";
    json << "    \"uptimeSeconds\": " << optional_double(status.websocket.uptime_seconds, 2)
         << "\n";
    json << "  },\n";
    json << "  \"battery\": {\n";
    json << "    \"present\": " << (status.battery.present ? "true" : "false") << ",\n";
    if (status.battery.percentage >= 0) {
        json << "    \"percentage\": " << status.battery.percentage << ",\n";
    } else {
        json << "    \"percentage\": null,\n";
    }
    json << "    \"state\": \"" << json_escape(status.battery.state) << "\"\n";
    json << "  },\n";
    json << "  \"debian\": {\n";
    json << "    \"uptimeSeconds\": " << optional_double(status.debian.uptime_seconds, 2) << ",\n";
    json << "    \"uptimeHuman\": \"" << json_escape(status.debian.uptime_human) << "\",\n";
    json << "    \"bootTime\": \"" << json_escape(status.debian.boot_time_iso) << "\",\n";
    json << "    \"loadAverage\": [" << format_double(status.debian.load_average[0], 2) << ", "
         << format_double(status.debian.load_average[1], 2) << ", "
         << format_double(status.debian.load_average[2], 2) << "]\n";
    json << "  },\n";
    json << "  \"network\": {\n";
    json << "    \"listeningPorts\": [";
    for (std::size_t i = 0; i < status.network.listening_ports.size(); ++i) {
        if (i > 0) {
            json << ", ";
        }
        json << status.network.listening_ports[i];
    }
    json << "]\n";
    json << "  }\n";
    json << "}\n";
    return json.str();
}

