#ifndef HTML_GENERATOR_H
#define HTML_GENERATOR_H

#include <string>
#include <vector>

// Structure to hold application tile information
struct AppTile {
    std::string name;
    std::string accent;
    std::string icon; // Can be an icon name or keyword
};

// Function to get sample application data (can be replaced with actual data loading)
std::vector<AppTile> get_sample_apps();

// Function to generate HTML for a single application tile
std::string generate_app_tile_html(const AppTile& app);

// Function to generate the complete HTML page for the menu
std::string generate_menu_page_html(const std::vector<AppTile>& apps);

// Function to generate JSON data for the applications
std::string generate_menu_json(const std::vector<AppTile>& apps);

#endif // HTML_GENERATOR_H
