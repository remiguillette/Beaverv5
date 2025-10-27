#include "core/app_manager.h"

#include <sstream>

#include "ui/html_renderer.h"

AppManager::AppManager()
    : apps_({
          {"BeaverPhone", "violet", "Phone"},
          {"BeaverSystem", "cyan", "Settings"},
          {"BeaverAlarm", "amber", "Alarm"},
          {"BeaverTask", "red", "ClipboardCheck"},
          {"BeaverDoc", "green", "FileText"},
          {"BeaverDebian", "violet", "Linux"},
          {"BeaverNet", "amber", "Globe"}}) {}

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
