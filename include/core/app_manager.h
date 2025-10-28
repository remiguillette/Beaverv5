#pragma once

#include <string>
#include <vector>

#include "core/language.h"
#include "core/translation_catalog.h"

struct AppRoutes {
    std::string kiosk;
    std::string http;
};

struct AppTile {
    std::string name;
    std::string accent;
    std::string icon;
    AppRoutes routes;
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

class AppManager {
public:
    AppManager();

    const std::vector<AppTile>& get_available_apps() const;

    void set_default_language(Language language);
    Language get_default_language() const;

    void set_app_routes(const std::string& app_name, const AppRoutes& routes);

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
    std::string beaversystem_page_html() const;
    std::string beaversystem_page_html(Language language,
                                       BeaverSystemMenuLinkMode menu_link_mode =
                                           BeaverSystemMenuLinkMode::kAbsoluteRoot) const;
    std::string beaversystem_page_html(Language language, const std::string& asset_prefix) const;
    std::string beaversystem_page_html(Language language, const std::string& asset_prefix,
                                       BeaverSystemMenuLinkMode menu_link_mode =
                                           BeaverSystemMenuLinkMode::kAbsoluteRoot) const;

private:
    std::vector<AppTile> apps_;
    Language default_language_;
    TranslationCatalog translation_catalog_;
};
