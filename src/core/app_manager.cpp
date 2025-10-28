#include "core/app_manager.h"

#include <algorithm>
#include <filesystem>
#include <sstream>

#include "core/system_status.h"
#include "ui/html_renderer.h"
#include <glib.h>

namespace {
std::string locale_directory() {
    namespace fs = std::filesystem;
    return (fs::current_path() / "locales").string();
}

}  // namespace

AppManager::AppManager()
    : apps_({
          {"BeaverPhone", "violet", "icons/phone.svg",
           {"apps/beaverphone", "/apps/beaverphone"}},
          {"BeaverSystem", "cyan", "icons/server.svg", {"apps/beaversystem", "/apps/beaversystem"}},
          {"BeaverAlarm", "amber", "icons/shield-alert.svg", {"", ""}},
          {"BeaverTask", "red", "icons/square-check-big.svg", {"", ""}},
          {"BeaverDoc", "green", "icons/file-text.svg",
           {"http://localhost:8000", "http://192.168.1.76:8000"}},
          {"BeaverDebian", "violet", "icons/server-cog.svg",
           {"http://localhost:9090/", "http://192.168.1.76:9090/"}},
          {"BeaverNet", "amber", "icons/chromium.svg", {"", ""}}}),
      default_language_(Language::French),
      translation_catalog_(locale_directory()) {
    g_message("AppManager initialized with %zu apps. default_language=%s", apps_.size(),
              language_to_string(default_language_));
}

const std::vector<AppTile>& AppManager::get_available_apps() const {
    return apps_;
}

void AppManager::set_app_routes(const std::string& app_name, const AppRoutes& routes) {
    auto it = std::find_if(apps_.begin(), apps_.end(), [&](const AppTile& tile) {
        return tile.name == app_name;
    });

    if (it == apps_.end()) {
        g_warning("AppManager could not find app named '%s' when attempting to update routes.",
                  app_name.c_str());
        return;
    }

    it->routes = routes;
    g_message("AppManager updated routes for '%s'. kiosk=%s http=%s", app_name.c_str(),
              routes.kiosk.c_str(), routes.http.c_str());
}

void AppManager::set_default_language(Language language) {
    default_language_ = language;
    g_message("AppManager default language set to %s", language_to_string(language));
}

Language AppManager::get_default_language() const {
    return default_language_;
}

std::string AppManager::to_json() const {
    return to_json(default_language_);
}

std::string AppManager::to_html() const {
    return to_html(default_language_, "", MenuRouteMode::kKiosk);
}

std::string AppManager::to_html(Language language) const {
    return to_html(language, "", MenuRouteMode::kKiosk);
}

std::string AppManager::to_html(Language language, MenuRouteMode route_mode) const {
    return to_html(language, "", route_mode);
}

std::string AppManager::to_json(Language language) const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"apps\": [\n";

    for (std::size_t i = 0; i < apps_.size(); ++i) {
        const auto& app = apps_[i];
        const std::string localized_name = translation_catalog_.translate(app.name, language);
        json << "    {\n";
        json << "      \"name\": \"" << localized_name << "\",\n";
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

std::string AppManager::to_html(Language language, const std::string& asset_prefix,
                                MenuRouteMode route_mode) const {
    std::string html = generate_menu_page_html(apps_, translation_catalog_, language, route_mode,
                                               asset_prefix);
    if (html.empty()) {
        g_warning("AppManager generated empty menu HTML for language: %s",
                  language_to_string(language));
    } else {
        g_message("AppManager generated menu HTML. language=%s bytes=%zu",
                  language_to_string(language), html.size());
    }
    return html;
}

std::string AppManager::beaverphone_page_html() const {
    return beaverphone_page_html(default_language_,
                                 BeaverphoneMenuLinkMode::kAbsoluteRoot);
}

std::string AppManager::beaverphone_page_html(Language language,
                                              BeaverphoneMenuLinkMode menu_link_mode) const {
    return beaverphone_page_html(language, "", menu_link_mode);
}

std::string AppManager::beaverphone_page_html(Language language,
                                              const std::string& asset_prefix) const {
    return beaverphone_page_html(language, asset_prefix,
                                 BeaverphoneMenuLinkMode::kAbsoluteRoot);
}

std::string AppManager::beaverphone_page_html(Language language,
                                              const std::string& asset_prefix,
                                              BeaverphoneMenuLinkMode menu_link_mode) const {
    std::string html = generate_beaverphone_dialpad_html(translation_catalog_, language,
                                                         asset_prefix, menu_link_mode);
    if (html.empty()) {
        g_warning("AppManager generated empty BeaverPhone HTML for language: %s",
                  language_to_string(language));
    } else {
        g_message("AppManager generated BeaverPhone HTML. language=%s bytes=%zu",
                  language_to_string(language), html.size());
    }
    return html;
}

std::string AppManager::beaversystem_page_html() const {
    return beaversystem_page_html(default_language_, BeaverSystemMenuLinkMode::kAbsoluteRoot);
}

std::string AppManager::beaversystem_page_html(Language language,
                                               BeaverSystemMenuLinkMode menu_link_mode) const {
    return beaversystem_page_html(language, "", menu_link_mode);
}

std::string AppManager::beaversystem_page_html(Language language,
                                               const std::string& asset_prefix) const {
    return beaversystem_page_html(language, asset_prefix,
                                  BeaverSystemMenuLinkMode::kAbsoluteRoot);
}

std::string AppManager::beaversystem_page_html(Language language,
                                               const std::string& asset_prefix,
                                               BeaverSystemMenuLinkMode menu_link_mode) const {
    SystemStatusSnapshot snapshot = collect_system_status();
    std::string html = generate_beaversystem_dashboard_html(translation_catalog_, language,
                                                            asset_prefix, menu_link_mode,
                                                            snapshot);
    if (html.empty()) {
        g_warning("AppManager generated empty BeaverSystem HTML for language: %s",
                  language_to_string(language));
    } else {
        g_message("AppManager generated BeaverSystem HTML. language=%s bytes=%zu",
                  language_to_string(language), html.size());
    }
    return html;
}
