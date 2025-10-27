#ifndef HTML_GENERATOR_H
#define HTML_GENERATOR_H

#include <string>
#include <vector>
#include <sstream>

struct AppTile {
    std::string name;
    std::string accent;
    std::string icon;
};

std::string generate_app_tile_html(const AppTile& app);
std::string generate_menu_page_html(const std::vector<AppTile>& apps);
std::string generate_menu_json(const std::vector<AppTile>& apps);
std::vector<AppTile> get_sample_apps();

#endif
