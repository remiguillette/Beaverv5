#include "core/cctv_config.h"

#include <mutex>
#include <sstream>
#include <string>

#include <glib.h>

namespace {

constexpr const char kCameraHost[] = "192.168.1.47";
constexpr const char kDirectRtspUri[] =
    "rtsp://admin:MC44rg99qc%40@192.168.1.47:554/cam/realmonitor?channel=1&subtype=1";
constexpr const char kOnvifPath[] = "onvif/ptz_service";
constexpr const char kOnvifProfile[] = "Profile_1";
constexpr const char kPtzUsername[] = "admin";
constexpr const char kPtzPassword[] = "MC44rg99qc@";
constexpr const char kMjpegStreamUrl[] = "http://localhost:8090/feed.mjpeg";

}  // namespace

bool CctvConfig::is_ready() const noexcept {
    const bool has_direct_rtsp = !rtsp_path.empty() &&
                                 (rtsp_path.rfind("rtsp://", 0) == 0 ||
                                  rtsp_path.rfind("rtsps://", 0) == 0);
    const bool has_host = !camera_host.empty();
    const bool has_hls = !hls_playlist_url.empty();
    const bool has_mjpeg = !mjpeg_stream_url.empty();
    return has_direct_rtsp || has_host || has_hls || has_mjpeg;
}

bool CctvConfig::ptz_is_ready() const noexcept {
    return !camera_host.empty() && !username.empty() && !password.empty();
}

std::string CctvConfig::rtsp_uri(bool include_credentials) const {
    const auto is_absolute_rtsp = [](const std::string& value) {
        return value.rfind("rtsp://", 0) == 0 || value.rfind("rtsps://", 0) == 0;
    };

    if (is_absolute_rtsp(rtsp_path)) {
        if (!include_credentials || username.empty()) {
            return rtsp_path;
        }

        const std::size_t scheme_end = rtsp_path.find("://");
        if (scheme_end == std::string::npos) {
            return rtsp_path;
        }

        const std::size_t authority_start = scheme_end + 3;
        const std::size_t authority_end = rtsp_path.find('@', authority_start);
        if (authority_end != std::string::npos) {
            return rtsp_path;
        }

        std::ostringstream uri;
        uri << rtsp_path.substr(0, authority_start);
        uri << username;
        if (!password.empty()) {
            uri << ':' << password;
        }
        uri << '@';
        uri << rtsp_path.substr(authority_start);
        return uri.str();
    }

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
    config.camera_host = kCameraHost;
    config.rtsp_path = kDirectRtspUri;
    config.onvif_path = kOnvifPath;
    config.username = kPtzUsername;
    config.password = kPtzPassword;
    config.profile_token = kOnvifProfile;
    config.hls_playlist_url.clear();
    config.mjpeg_stream_url = kMjpegStreamUrl;
    config.streaming_protocol = "MJPEG";

    static std::once_flag log_once;
    std::call_once(log_once, [&config]() {
        const std::string direct_rtsp = config.rtsp_uri(false);
        g_message("Loaded BeaverAlarm CCTV host: %s", config.camera_host.c_str());
        if (!direct_rtsp.empty()) {
            g_message("Using direct RTSP URI for BeaverAlarm CCTV feed: %s", direct_rtsp.c_str());
        }

        if (!config.profile_token.empty()) {
            g_message("Using ONVIF profile token: %s", config.profile_token.c_str());
        }

        if (!config.mjpeg_stream_url.empty()) {
            g_message("MJPEG stream URL available for BeaverAlarm CCTV: %s",
                      config.mjpeg_stream_url.c_str());
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
