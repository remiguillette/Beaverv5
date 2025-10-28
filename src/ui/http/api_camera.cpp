#include "ui/http/api_camera.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <mutex>
#include <sstream>

#include <glib.h>

#include "core/cctv_config.h"
#include "core/ptz_controller.h"

namespace {

PtzController& global_ptz_controller() {
    static std::once_flag once;
    static std::unique_ptr<PtzController> controller;
    std::call_once(once, []() {
        CctvConfig config = load_cctv_config_from_env();
        controller = std::make_unique<PtzController>(config);
    });
    return *controller;
}

CommandResult dispatch_ptz_action(PtzController& controller, const std::string& action) {
    if (action == "left") {
        return controller.pan_left();
    }
    if (action == "right") {
        return controller.pan_right();
    }
    if (action == "up") {
        return controller.tilt_up();
    }
    if (action == "down") {
        return controller.tilt_down();
    }
    if (action == "zoom_in") {
        return controller.zoom_in();
    }
    if (action == "zoom_out") {
        return controller.zoom_out();
    }
    if (action == "stop") {
        return controller.stop();
    }
    return CommandResult::Error("Unknown PTZ action");
}

std::string action_from_request(const std::map<std::string, std::string>& query_parameters,
                                const HttpRequest& request) {
    auto it = query_parameters.find("action");
    if (it != query_parameters.end()) {
        return it->second;
    }

    const std::string prefix = "action=";
    const std::string& body = request.body;
    const auto pos = body.find(prefix);
    if (pos != std::string::npos) {
        std::string value = body.substr(pos + prefix.size());
        auto end = value.find_first_of("&\n\r");
        if (end != std::string::npos) {
            value = value.substr(0, end);
        }
        return value;
    }
    return {};
}

}  // namespace

bool handle_camera_api(const std::string& path, const HttpRequest& request,
                       const std::map<std::string, std::string>& query_parameters,
                       HttpResponse& response) {
    if (path != "/api/ptz" && path != "/api/cctv/stream") {
        return false;
    }

    if (path == "/api/cctv/stream") {
        CctvConfig config = load_cctv_config_from_env();
        response.headers["Content-Type"] = "application/json; charset=utf-8";
        response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
        response.headers["Access-Control-Allow-Origin"] = "*";

        std::ostringstream json;
        json << "{\n";
        json << "  \"protocol\": \"" << config.streaming_protocol << "\",\n";
        json << "  \"playlist\": \"" << config.hls_playlist_url << "\",\n";
        json << "  \"rtsp\": \"" << config.rtsp_uri(false) << "\"\n";
        json << "}\n";

        response.body = json.str();
        return true;
    }

    if (request.method != "POST") {
        response.status_code = 405;
        response.status_text = "Method Not Allowed";
        response.headers["Allow"] = "POST";
        response.body = "Method Not Allowed";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return true;
    }

    std::string action = action_from_request(query_parameters, request);
    if (action.empty()) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "Missing action parameter";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return true;
    }

    std::transform(action.begin(), action.end(), action.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    PtzController& controller = global_ptz_controller();
    CommandResult result = dispatch_ptz_action(controller, action);

    response.headers["Content-Type"] = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
    response.headers["Access-Control-Allow-Origin"] = "*";

    std::ostringstream json;
    json << "{\n";
    json << "  \"action\": \"" << action << "\",\n";
    json << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
    json << "  \"message\": \"" << result.message << "\"\n";
    json << "}\n";

    response.body = json.str();

    if (!result.success) {
        response.status_code = 502;
        response.status_text = "Bad Gateway";
    }

    return true;
}

