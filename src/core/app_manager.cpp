#include "core/app_manager.h"

#include <filesystem>
#include <sstream>

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
          {"BeaverSystem", "cyan", "icons/server.svg", {"", ""}},
          {"BeaverAlarm", "amber", "icons/shield-alert.svg", {"", ""}},
          {"BeaverTask", "red", "icons/square-check-big.svg", {"", ""}},
          {"BeaverDoc", "green", "icons/file-text.svg", {"", ""}},
          {"BeaverDebian", "violet", "icons/server-cog.svg", {"", ""}},
          {"BeaverNet", "amber", "icons/chromium.svg", {"", ""}}}),
      default_language_(Language::French),
      translation_catalog_(locale_directory()) {
    g_message("AppManager initialized with %zu apps. default_language=%s", apps_.size(),
              language_to_string(default_language_));
}

const std::vector<AppTile>& AppManager::get_available_apps() const {
    return apps_;
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
