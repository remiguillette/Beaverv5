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

std::string extract_origin(const std::string& uri) {
    if (uri.empty()) {
        return "";
    }

    GUri* parsed_uri = g_uri_parse(uri.c_str(), G_URI_FLAGS_NONE, nullptr);
    if (parsed_uri == nullptr) {
        return "";
    }

    const gchar* scheme = g_uri_get_scheme(parsed_uri);
    const gchar* host = g_uri_get_host(parsed_uri);
    const gint port = g_uri_get_port(parsed_uri);

    std::string origin;
    if (scheme != nullptr && scheme[0] != '\0' && host != nullptr && host[0] != '\0') {
        origin.assign(scheme);
        origin.append("://");
        origin.append(host);
        if (port > 0) {
            origin.push_back(':');
            origin.append(std::to_string(port));
        }
    }

    g_uri_unref(parsed_uri);
    return origin;
}

RouteEntry make_route_entry(const std::string& uri, bool remote) {
    RouteEntry entry;
    entry.uri = uri;
    entry.remote = remote;
    entry.origin = extract_origin(uri);
    return entry;
}

void normalize_route_entry(RouteEntry& entry) {
    entry.origin = extract_origin(entry.uri);
}

}  // namespace

AppManager::AppManager()
    : apps_({
          {"BeaverPhone", "violet", "icons/phone.svg",
           {make_route_entry("apps/beaverphone", false),
            make_route_entry("/apps/beaverphone", false)}},
          {"BeaverSystem", "cyan", "icons/server.svg",
           {make_route_entry("apps/beaversystem", false),
            make_route_entry("/apps/beaversystem", false)}},
          {"BeaverAlarm", "amber", "icons/shield-alert.svg",
           {make_route_entry("", false), make_route_entry("", false)}},
          {"BeaverTask", "red", "icons/square-check-big.svg",
           {make_route_entry("", false), make_route_entry("", false)}},
          {"BeaverDoc", "green", "icons/file-text.svg",
           {make_route_entry("http://localhost:8000", false),
            make_route_entry("http://192.168.1.76:8000", false)}},
          {"BeaverDebian", "violet", "icons/server-cog.svg",
           {make_route_entry("http://localhost:9090/", false),
            make_route_entry("http://192.168.1.76:9090/", false)}},
          {"BeaverNet", "amber", "icons/chromium.svg",
           {make_route_entry("https://rgbeavernet.ca/", true),
            make_route_entry("https://rgbeavernet.ca/", true)}}}),
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

    AppRoutes normalized = routes;
    normalize_route_entry(normalized.kiosk);
    normalize_route_entry(normalized.http);
    it->routes = normalized;
    g_message("AppManager updated routes for '%s'. kiosk=%s http=%s", app_name.c_str(),
              normalized.kiosk.uri.c_str(), normalized.http.uri.c_str());
}

void AppManager::record_navigation(const std::string& app_name, MenuRouteMode route_mode) {
    if (!navigation_history_.empty()) {
        const NavigationRecord& previous = navigation_history_.back();
        if (previous.app_name == app_name && previous.route_mode == route_mode) {
            g_message("AppManager navigation unchanged (app=%s mode=%s).", app_name.c_str(),
                      route_mode == MenuRouteMode::kKiosk ? "kiosk" : "http");
            return;
        }
    }

    navigation_history_.push_back({app_name, route_mode});
    g_message("AppManager recorded navigation. app=%s mode=%s", app_name.c_str(),
              route_mode == MenuRouteMode::kKiosk ? "kiosk" : "http");
}

void AppManager::clear_navigation_history() {
    if (!navigation_history_.empty()) {
        g_message("AppManager clearing %zu navigation records.", navigation_history_.size());
    }
    navigation_history_.clear();
}

std::optional<RouteMatch> AppManager::match_route_for_uri(const std::string& uri,
                                                          MenuRouteMode route_mode) const {
    const std::string origin = extract_origin(uri);
    if (origin.empty()) {
        return std::nullopt;
    }

    for (const auto& app : apps_) {
        const RouteEntry* entry = nullptr;
        switch (route_mode) {
            case MenuRouteMode::kKiosk:
                entry = &app.routes.kiosk;
                break;
            case MenuRouteMode::kHttpServer:
            default:
                entry = &app.routes.http;
                break;
        }

        if (entry == nullptr || !entry->remote) {
            continue;
        }

        if (!entry->origin.empty() && entry->origin == origin) {
            return RouteMatch{&app, entry};
        }
    }

    return std::nullopt;
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
