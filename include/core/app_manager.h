#pragma once

#include <string>
#include <vector>

struct AppTile {
    std::string name;
    std::string accent;
    std::string icon;
};

enum class Language {
    French,
    English,
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

private:
    std::vector<AppTile> apps_;
    Language default_language_;
};
