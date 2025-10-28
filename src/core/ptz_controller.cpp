#include "core/ptz_controller.h"

#include <cmath>
#include <memory>
#include <mutex>
#include <sstream>
#include <utility>

#include <glib.h>

#if __has_include(<curl/curl.h>)
#define BEAVER_ALARM_HAS_CURL 1
#include <curl/curl.h>
#else
#define BEAVER_ALARM_HAS_CURL 0
#endif

namespace {

#if BEAVER_ALARM_HAS_CURL
void ensure_curl_initialized() {
    static std::once_flag once;
    std::call_once(once, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        g_message("libcurl initialized for PTZ controller");
    });
}
#endif

std::string build_continuous_move_envelope(const std::string& profile_token, double pan, double tilt,
                                           double zoom) {
    std::ostringstream soap;
    soap << "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\""
            " xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\""
            " xmlns:tt=\"http://www.onvif.org/ver10/schema\">";
    soap << "<s:Body>";
    soap << "<tptz:ContinuousMove>";
    soap << "<tptz:ProfileToken>" << profile_token << "</tptz:ProfileToken>";
    soap << "<tptz:Velocity>";
    if (std::abs(pan) > 1e-6 || std::abs(tilt) > 1e-6) {
        soap << "<tt:PanTilt x=\"" << pan << "\" y=\"" << tilt << "\"/>";
    }
    if (std::abs(zoom) > 1e-6) {
        soap << "<tt:Zoom x=\"" << zoom << "\"/>";
    }
    soap << "</tptz:Velocity>";
    soap << "</tptz:ContinuousMove>";
    soap << "</s:Body>";
    soap << "</s:Envelope>";
    return soap.str();
}

std::string build_stop_envelope(const std::string& profile_token, bool pan_tilt, bool zoom) {
    std::ostringstream soap;
    soap << "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\""
            " xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\">";
    soap << "<s:Body>";
    soap << "<tptz:Stop>";
    soap << "<tptz:ProfileToken>" << profile_token << "</tptz:ProfileToken>";
    soap << "<tptz:PanTilt>" << (pan_tilt ? "true" : "false") << "</tptz:PanTilt>";
    soap << "<tptz:Zoom>" << (zoom ? "true" : "false") << "</tptz:Zoom>";
    soap << "</tptz:Stop>";
    soap << "</s:Body>";
    soap << "</s:Envelope>";
    return soap.str();
}

}  // namespace

CommandResult CommandResult::Ok(std::string msg) {
    return CommandResult{true, std::move(msg)};
}

CommandResult CommandResult::Error(std::string msg) {
    return CommandResult{false, std::move(msg)};
}

PtzController::PtzController(const CctvConfig& config) : config_(config) {
    if (!config_.is_ready()) {
        g_warning("PTZ controller initialized with incomplete configuration. host=%s user=%s",\
                  config_.camera_host.c_str(), sanitize_for_logging(config_.username).c_str());
    }
#if BEAVER_ALARM_HAS_CURL
    ensure_curl_initialized();
#else
    g_warning("libcurl not available; PTZ commands will be logged but not sent.");
#endif
}

CommandResult PtzController::pan_left() {
    return send_continuous_move(-pan_speed_, 0.0, 0.0);
}

CommandResult PtzController::pan_right() {
    return send_continuous_move(pan_speed_, 0.0, 0.0);
}

CommandResult PtzController::tilt_up() {
    return send_continuous_move(0.0, tilt_speed_, 0.0);
}

CommandResult PtzController::tilt_down() {
    return send_continuous_move(0.0, -tilt_speed_, 0.0);
}

CommandResult PtzController::zoom_in() {
    return send_continuous_move(0.0, 0.0, zoom_speed_);
}

CommandResult PtzController::zoom_out() {
    return send_continuous_move(0.0, 0.0, -zoom_speed_);
}

CommandResult PtzController::stop() {
    return send_stop(true, true);
}

CommandResult PtzController::send_continuous_move(double pan, double tilt, double zoom) {
    if (!config_.is_ready()) {
        return CommandResult::Error("CCTV configuration incomplete");
    }

    const std::string body =
        build_continuous_move_envelope(config_.profile_token, pan, tilt, zoom);
    auto result = send_soap_request(body, "http://www.onvif.org/ver20/ptz/wsdl/ContinuousMove");
    if (result.success) {
        g_message("Issued PTZ ContinuousMove pan=%0.2f tilt=%0.2f zoom=%0.2f", pan, tilt, zoom);
    }
    return result;
}

CommandResult PtzController::send_stop(bool pan_tilt, bool zoom) {
    if (!config_.is_ready()) {
        return CommandResult::Error("CCTV configuration incomplete");
    }

    const std::string body = build_stop_envelope(config_.profile_token, pan_tilt, zoom);
    auto result = send_soap_request(body, "http://www.onvif.org/ver20/ptz/wsdl/Stop");
    if (result.success) {
        g_message("Issued PTZ Stop pan_tilt=%d zoom=%d", pan_tilt ? 1 : 0, zoom ? 1 : 0);
    }
    return result;
}

CommandResult PtzController::send_soap_request(const std::string& body,
                                               const std::string& soap_action) {
    if (!config_.is_ready()) {
        return CommandResult::Error("CCTV configuration incomplete");
    }

    const std::string endpoint = config_.onvif_endpoint();
    if (endpoint.empty()) {
        return CommandResult::Error("ONVIF endpoint missing");
    }

#if BEAVER_ALARM_HAS_CURL
    ensure_curl_initialized();
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        return CommandResult::Error("Failed to initialize libcurl");
    }

    struct CurlCleanup {
        CURL* handle;
        explicit CurlCleanup(CURL* h) : handle(h) {}
        ~CurlCleanup() { curl_easy_cleanup(handle); }
    } cleanup{curl};

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_USERNAME, config_.username.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, config_.password.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000L);

    struct curl_slist* headers = nullptr;
    std::ostringstream content_type;
    content_type << "Content-Type: application/soap+xml; charset=utf-8";
    headers = curl_slist_append(headers, content_type.str().c_str());
    std::ostringstream soap_action_header;
    soap_action_header << "SOAPAction: \"" << soap_action << "\"";
    headers = curl_slist_append(headers, soap_action_header.str().c_str());
    headers = curl_slist_append(headers, "Connection: close");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    auto headers_cleanup = [](struct curl_slist* list) {
        if (list != nullptr) {
            curl_slist_free_all(list);
        }
    };
    std::unique_ptr<curl_slist, decltype(headers_cleanup)> header_guard(headers, headers_cleanup);

    CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        return CommandResult::Error(curl_easy_strerror(code));
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code >= 200 && http_code < 300) {
        return CommandResult::Ok("PTZ command acknowledged");
    }

    std::ostringstream error;
    error << "PTZ HTTP error " << http_code;
    return CommandResult::Error(error.str());
#else
    g_message("PTZ SOAP command prepared for endpoint=%s action=%s", endpoint.c_str(),
              soap_action.c_str());
    (void)body;  // suppress unused warning when libcurl missing
    return CommandResult::Error("libcurl unavailable");
#endif
}

