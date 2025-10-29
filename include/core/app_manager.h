#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/language.h"
#include "core/translation_catalog.h"

struct RouteEntry {
    std::string uri;
    bool remote;
    std::string origin;
};

struct AppRoutes {
    RouteEntry kiosk;
    RouteEntry http;
};

struct AppTile {
    std::string name;
    std::string accent;
    std::string icon;
    AppRoutes routes;
};

struct RouteMatch {
    const AppTile* app;
    const RouteEntry* route;
};

enum class MenuRouteMode {
    kKiosk,
    kHttpServer,
};

enum class BeaverphoneMenuLinkMode {
    kAbsoluteRoot,
    kRelativeIndex
};

enum class BeaverSystemMenuLinkMode {
    kAbsoluteRoot,
    kRelativeIndex
};

enum class BeaverAlarmMenuLinkMode {
    kAbsoluteRoot,
    kRelativeIndex
};

enum class BeaverTaskMenuLinkMode {
    kAbsoluteRoot,
    kRelativeIndex
};

struct NavigationRecord {
    std::string app_name;
    MenuRouteMode route_mode;
};

class AppManager {
public:
    AppManager();

    const std::vector<AppTile>& get_available_apps() const;

    void set_default_language(Language language);
    Language get_default_language() const;

    void set_app_routes(const std::string& app_name, const AppRoutes& routes);

    void record_navigation(const std::string& app_name, MenuRouteMode route_mode);
    void clear_navigation_history();
    std::optional<RouteMatch> match_route_for_uri(const std::string& uri,
                                                  MenuRouteMode route_mode) const;

    std::string to_json() const;
    std::string to_json(Language language) const;
    std::string to_html() const;
    std::string to_html(Language language) const;
    std::string to_html(Language language, MenuRouteMode route_mode) const;
    std::string to_html(Language language, const std::string& asset_prefix,
                        MenuRouteMode route_mode = MenuRouteMode::kHttpServer) const;
    std::string beaverphone_page_html() const;
    std::string beaverphone_page_html(Language language,
                                      BeaverphoneMenuLinkMode menu_link_mode =
                                          BeaverphoneMenuLinkMode::kAbsoluteRoot) const;
    std::string beaverphone_page_html(Language language,
                                      const std::string& asset_prefix) const;
    std::string beaverphone_page_html(Language language, const std::string& asset_prefix,
                                      BeaverphoneMenuLinkMode menu_link_mode =
                                          BeaverphoneMenuLinkMode::kAbsoluteRoot) const;
    std::string beaveralarm_page_html() const;
    std::string beaveralarm_page_html(Language language,
                                      BeaverAlarmMenuLinkMode menu_link_mode =
                                          BeaverAlarmMenuLinkMode::kAbsoluteRoot) const;
    std::string beaveralarm_page_html(Language language, const std::string& asset_prefix) const;
    std::string beaveralarm_page_html(Language language, const std::string& asset_prefix,
                                      BeaverAlarmMenuLinkMode menu_link_mode =
                                          BeaverAlarmMenuLinkMode::kAbsoluteRoot) const;
    std::string beaversystem_page_html() const;
    std::string beaversystem_page_html(Language language,
                                       BeaverSystemMenuLinkMode menu_link_mode =
                                           BeaverSystemMenuLinkMode::kAbsoluteRoot) const;
    std::string beaversystem_page_html(Language language, const std::string& asset_prefix) const;
    std::string beaversystem_page_html(Language language, const std::string& asset_prefix,
                                       BeaverSystemMenuLinkMode menu_link_mode =
                                           BeaverSystemMenuLinkMode::kAbsoluteRoot) const;
    std::string beavertask_page_html() const;
    std::string beavertask_page_html(
        Language language,
        BeaverTaskMenuLinkMode menu_link_mode = BeaverTaskMenuLinkMode::kAbsoluteRoot) const;
    std::string beavertask_page_html(Language language, const std::string& asset_prefix,
                                     BeaverTaskMenuLinkMode menu_link_mode =
                                         BeaverTaskMenuLinkMode::kAbsoluteRoot) const;
private:
    std::vector<AppTile> apps_;
    Language default_language_;
    TranslationCatalog translation_catalog_;
    std::vector<NavigationRecord> navigation_history_;
};
