#include "core/app_manager.h"

#include <sstream>

#include "ui/html_renderer.h"

AppManager::AppManager()
    : apps_({
          {"BeaverPhone", "violet", "icons/phone.svg"},
          {"BeaverSystem", "cyan", "icons/server.svg"},
          {"BeaverAlarm", "amber", "icons/shield-alert.svg"},
          {"BeaverTask", "red", "icons/square-check-big.svg"},
          {"BeaverDoc", "green", "icons/file-text.svg"},
          {"BeaverDebian", "violet", "icons/server-cog.svg"},
          {"BeaverNet", "amber", "icons/chromium.svg"}}) {}

const std::vector<AppTile>& AppManager::get_available_apps() const {
    return apps_;
}

std::string AppManager::to_json() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"apps\": [\n";

    for (std::size_t i = 0; i < apps_.size(); ++i) {
        const auto& app = apps_[i];
        json << "    {\n";
        json << "      \"name\": \"" << app.name << "\",\n";
        json << "      \"accent\": \"" << app.accent << "\",\n";
        json << "      \"icon\": \"" << app.icon << "\"\n";
        json << "    }";
        if (i + 1 < apps_.size()) {
            json << ",";
        }
        json << "\n";
    }

    json << "  ]\n";
    json << "}\n";

    return json.str();
}

std::string AppManager::to_html() const {
    return generate_menu_page_html(apps_);
}
