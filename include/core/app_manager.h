#pragma once

#include <string>
#include <vector>

struct AppTile {
    std::string name;
    std::string accent;
    std::string icon;
};

class AppManager {
public:
    AppManager();

    const std::vector<AppTile>& get_available_apps() const;
    std::string to_json() const;
    std::string to_html() const;

private:
    std::vector<AppTile> apps_;
};
