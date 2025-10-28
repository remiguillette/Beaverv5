#pragma once

#include <string>
#include <vector>

#include "core/language.h"
#include "core/translation_catalog.h"

struct AppTile {
    std::string name;
    std::string accent;
    std::string icon;
    std::string route;
};

class AppManager {
public:
    AppManager();

    const std::vector<AppTile>& get_available_apps() const;

    void set_default_language(Language language);
    Language get_default_language() const;

    std::string to_json() const;
    std::string to_json(Language language) const;
    std::string to_html() const;
    std::string to_html(Language language) const;
    std::string to_html(Language language, const std::string& asset_prefix) const;
    std::string beaverphone_page_html() const;
    std::string beaverphone_page_html(Language language) const;
    std::string beaverphone_page_html(Language language,
                                      const std::string& asset_prefix) const;

private:
    std::vector<AppTile> apps_;
    Language default_language_;
    TranslationCatalog translation_catalog_;
};
