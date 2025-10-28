#pragma once

#include <cstddef>
#include <string>

struct CctvConfig {
    std::string camera_host;
    std::string rtsp_path;
    std::string onvif_path;
    std::string username;
    std::string password;
    std::string profile_token;
    std::string hls_playlist_url;
    std::string mjpeg_stream_url;
    std::string streaming_protocol;

    [[nodiscard]] bool is_ready() const noexcept;
    [[nodiscard]] bool ptz_is_ready() const noexcept;
    [[nodiscard]] std::string rtsp_uri(bool include_credentials = false) const;
    [[nodiscard]] std::string onvif_endpoint() const;
};

CctvConfig load_cctv_config_from_env();
std::string sanitize_for_logging(const std::string& value, std::size_t visible = 2);
