#include "ui/html_renderer.h"

#include <cctype>
#include <sstream>

std::string generate_app_tile_html(const AppTile& app) {
    std::ostringstream html;
    char initial = app.name.empty() ? 'A' : toupper(app.name[0]);

    html << "<button type=\"button\" class=\"app-tile app-tile--" << app.accent << "\">\n";
    html << "  <div class=\"app-tile__icon\" aria-hidden=\"true\">\n";
    html << "    <span class=\"app-tile__glyph\">" << initial << "</span>\n";
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
    html << "  <link rel=\"stylesheet\" href=\"css/styles.css\" />\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div id=\"root\">\n";
    html << "    <div class=\"menu-root\">\n";
    html << "      <header class=\"menu-header\">\n";
    html << "        <h1 class=\"menu-header__title\">\n";
    html << "          <span class=\"menu-header__welcome\">Welcome</span>\n";
    html << "          <span class=\"menu-header__connector\">to the</span>\n";
    html << "          <span class=\"menu-header__brand\">BeaverKiosk</span>\n";
    html << "        </h1>\n";
    html << "      </header>\n";
    html << "      <main class=\"menu-grid\">\n";
    
    for (const auto& app : apps) {
        html << "        " << generate_app_tile_html(app);
    }
    
    html << "      </main>\n";
    html << "    </div>\n";
    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}
