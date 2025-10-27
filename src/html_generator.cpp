#include "../include/html_generator.h"
#include <sstream> // For std::ostringstream
#include <cctype>  // For toupper
#include <vector>  // Ensure vector is included
#include <string>  // Ensure string is included

// Function to get sample application data
std::vector<AppTile> get_sample_apps() {
    // Returns a predefined list of sample applications
    return {
        {"Code Editor", "violet", "Code"},
        {"Terminal", "cyan", "Terminal"},
        {"File Manager", "amber", "Folder"},
        {"Web Browser", "red", "Globe"},
        {"Settings", "green", "Settings"},
        {"Music Player", "violet", "Music"},
        {"Calculator", "amber", "Calculator"},
        {"Calendar", "cyan", "Calendar"}
    };
}

// Function to generate HTML for a single application tile
std::string generate_app_tile_html(const AppTile& app) {
    std::ostringstream html;
    // Get the first character of the app name, convert to uppercase, default to 'A' if empty
    char initial = app.name.empty() ? 'A' : static_cast<char>(toupper(app.name[0]));

    // Construct the HTML button element for the app tile
    html << "<button type=\"button\" class=\"app-tile app-tile--" << app.accent << "\">\n";
    html << "  <div class=\"app-tile__icon\" aria-hidden=\"true\">\n";
    // Display the initial character as a simple icon placeholder
    html << "    <span style=\"font-size: 2.5rem; font-weight: 300;\">" << initial << "</span>\n";
    html << "  </div>\n";
    html << "  <h3 class=\"app-tile__name\">" << app.name << "</h3>\n";
    html << "</button>\n";

    return html.str(); // Return the generated HTML string
}

// Function to generate the complete HTML page for the menu
std::string generate_menu_page_html(const std::vector<AppTile>& apps) {
    std::ostringstream html;

    // Basic HTML document structure
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\" />\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";
    html << "  <title>BeaverKiosk - C++ Edition</title>\n";
    // Link Google Fonts
    html << "  <link rel=\"preconnect\" href=\"https://fonts.googleapis.com\" />\n";
    html << "  <link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin />\n";
    html << "  <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap\" rel=\"stylesheet\" />\n";
    // Link local CSS stylesheet (assuming it will be served at this path)
    html << "  <link rel=\"stylesheet\" href=\"/css/styles.css\" />\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div id=\"root\">\n";
    html << "    <div class=\"menu-root\">\n";
    html << "      <header class=\"menu-header\">\n";
    html << "        <h1>BeaverKiosk</h1>\n";
    html << "        <p class=\"subtitle\">Unified application launcher • C++ Edition</p>\n";
    html << "      </header>\n";
    html << "      <main class=\"menu-grid\">\n";

    // Iterate through the apps and generate HTML for each tile
    for (const auto& app : apps) {
        html << "        " << generate_app_tile_html(app); // Indent for readability
    }

    html << "      </main>\n";
    html << "      <footer class=\"menu-footer\">\n";
    html << "        <small>C++ • Native HTTP Server • Beaver Suite ©2025</small>\n";
    html << "      </footer>\n";
    html << "    </div>\n";
    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str(); // Return the complete HTML page string
}

// Function to generate JSON data for the applications
std::string generate_menu_json(const std::vector<AppTile>& apps) {
    std::ostringstream json;

    json << "{\n";
    json << "  \"apps\": [\n";

    // Iterate through the apps and generate JSON objects
    for (size_t i = 0; i < apps.size(); ++i) {
        json << "    {\n";
        json << "      \"name\": \"" << apps[i].name << "\",\n";
        json << "      \"accent\": \"" << apps[i].accent << "\",\n";
        json << "      \"icon\": \"" << apps[i].icon << "\"\n";
        json << "    }";

        // Add a comma if it's not the last element
        if (i < apps.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << "  ]\n";
    json << "}\n";

    return json.str(); // Return the generated JSON string
}
