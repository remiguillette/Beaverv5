#include "html_generator.h"
#include <sstream>

std::vector<AppTile> get_sample_apps() {
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

std::string generate_app_tile_html(const AppTile& app) {
    std::ostringstream html;
    char initial = app.name.empty() ? 'A' : toupper(app.name[0]);
    
    html << "<button type=\"button\" class=\"app-tile app-tile--" << app.accent << "\">\n";
    html << "  <div class=\"app-tile__icon\" aria-hidden=\"true\">\n";
    html << "    <span style=\"font-size: 2.5rem; font-weight: 300;\">" << initial << "</span>\n";
    html << "  </div>\n";
    html << "  <h3 class=\"app-tile__name\">" << app.name << "</h3>\n";
    html << "</button>\n";
    
    return html.str();
}

std::string generate_menu_page_html(const std::vector<AppTile>& apps) {
    std::ostringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\" />\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";
    html << "  <title>BeaverKiosk - C++ Edition</title>\n";
    html << "  <link rel=\"preconnect\" href=\"https://fonts.googleapis.com\" />\n";
    html << "  <link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin />\n";
    html << "  <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap\" rel=\"stylesheet\" />\n";
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
    
    for (const auto& app : apps) {
        html << "        " << generate_app_tile_html(app);
    }
    
    html << "      </main>\n";
    html << "      <footer class=\"menu-footer\">\n";
    html << "        <small>C++ • Native HTTP Server • Beaver Suite ©2025</small>\n";
    html << "      </footer>\n";
    html << "    </div>\n";
    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}

std::string generate_menu_json(const std::vector<AppTile>& apps) {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"apps\": [\n";
    
    for (size_t i = 0; i < apps.size(); ++i) {
        json << "    {\n";
        json << "      \"name\": \"" << apps[i].name << "\",\n";
        json << "      \"accent\": \"" << apps[i].accent << "\",\n";
        json << "      \"icon\": \"" << apps[i].icon << "\"\n";
        json << "    }";
        
        if (i < apps.size() - 1) {
            json << ",";
        }
        json << "\n";
    }
    
    json << "  ]\n";
    json << "}\n";
    
    return json.str();
}
