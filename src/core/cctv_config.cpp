#include "core/cctv_config.h"

#include <cstdlib>
#include <mutex>
#include <sstream>
#include <string>

#include <glib.h>

namespace {

std::string get_env_or_default(const char* name, const char* fallback = "") {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return fallback == nullptr ? std::string{} : std::string{fallback};
    }
    return std::string{value};
}

}  // namespace

bool CctvConfig::is_ready() const noexcept {
    return !camera_host.empty() && !username.empty() && !password.empty();
}

std::string CctvConfig::rtsp_uri(bool include_credentials) const {
    if (camera_host.empty()) {
        return {};
    }

    std::ostringstream uri;
    uri << "rtsp://";
    if (include_credentials && !username.empty()) {
        uri << username;
        if (!password.empty()) {
            uri << ":" << password;
        }
        uri << "@";
    }
    uri << camera_host;

    if (!rtsp_path.empty()) {
        if (rtsp_path.front() != '/') {
            uri << '/';
        }
        uri << rtsp_path;
    }

    return uri.str();
}

std::string CctvConfig::onvif_endpoint() const {
    if (camera_host.empty()) {
        return {};
    }

    std::ostringstream uri;
    uri << "http://" << camera_host;
    if (!onvif_path.empty()) {
        if (onvif_path.front() != '/') {
            uri << '/';
        }
        uri << onvif_path;
    }
    return uri.str();
}

CctvConfig load_cctv_config_from_env() {
    CctvConfig config;
    config.camera_host = get_env_or_default("BEAVER_ALARM_CCTV_HOST");
    config.rtsp_path = get_env_or_default("BEAVER_ALARM_CCTV_RTSP_PATH", "cam/realmonitor?channel=1&subtype=1");
    config.onvif_path = get_env_or_default("BEAVER_ALARM_ONVIF_PATH", "onvif/ptz_service");
    config.username = get_env_or_default("BEAVER_ALARM_CCTV_USERNAME");
    config.password = get_env_or_default("BEAVER_ALARM_CCTV_PASSWORD");
    config.profile_token = get_env_or_default("BEAVER_ALARM_ONVIF_PROFILE", "Profile_1");
    config.hls_playlist_url = get_env_or_default("BEAVER_ALARM_HLS_URL", "/streams/beaveralarm/index.m3u8");
    config.streaming_protocol = "HLS";

    static std::once_flag log_once;
    std::call_once(log_once, [&config]() {
        if (!config.camera_host.empty()) {
            g_message("Loaded BeaverAlarm CCTV host: %s", config.camera_host.c_str());
        } else {
            g_warning("BEAVER_ALARM_CCTV_HOST is not set; CCTV feed disabled.");
        }

        if (!config.profile_token.empty()) {
            g_message("Using ONVIF profile token: %s", config.profile_token.c_str());
        }

        if (config.hls_playlist_url.empty()) {
            g_warning("HLS playlist URL not configured; set BEAVER_ALARM_HLS_URL.");
        }
    });

    return config;
}

std::string sanitize_for_logging(const std::string& value, std::size_t visible) {
    if (value.empty()) {
        return "<empty>";
    }
    if (value.size() <= visible) {
        return std::string(value.size(), '*');
    }
    std::string sanitized(value.size(), '*');
    for (std::size_t i = 0; i < visible && i < value.size(); ++i) {
        sanitized[i] = value[i];
    }
    return sanitized;
}
