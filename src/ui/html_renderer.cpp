#include "ui/html_renderer.h"

#include <libintl.h>
#include <sstream>

namespace {
const char* html_lang_code(Language language) {
    switch (language) {
        case Language::French:
            return "fr";
        case Language::English:
        default:
            return "en";
    }
}

std::string language_toggle_button(const char* label, const char* href, const char* aria_label,
                                   bool active) {
    std::ostringstream html;
    html << "          <a href=\"" << href << "\" class=\"lang-toggle__button";
    if (active) {
        html << " lang-toggle__button--active";
    }
    html << "\" aria-pressed=\"" << (active ? "true" : "false") << "\" title=\"" << aria_label
         << "\">" << label << "</a>\n";
    return html.str();
}
}  // namespace

std::string generate_app_tile_html(const AppTile& app) {
    std::ostringstream html;

    html << "<button type=\"button\" class=\"app-tile app-tile--" << app.accent << "\">\n";
    html << "  <div class=\"app-tile__icon\" aria-hidden=\"true\">\n";
    html << "    <img src=\"" << app.icon
         << "\" alt=\"\" class=\"app-tile__icon-image\" loading=\"lazy\" />\n";
    html << "  </div>\n";
    html << "  <h3 class=\"app-tile__name\">" << gettext(app.name.c_str()) << "</h3>\n";
    html << "</button>\n";

    return html.str();
}

std::string generate_menu_page_html(const std::vector<AppTile>& apps, Language language) {
    std::ostringstream html;

    const char* lang_code = html_lang_code(language);
    const std::string language_label = gettext("Language selection");
    const std::string switch_to_french = gettext("Switch to French");
    const std::string switch_to_english = gettext("Switch to English");

    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"" << lang_code << "\">\n";
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
    html << "        <nav class=\"lang-toggle\" role=\"group\" aria-label=\"" << language_label
         << "\">\n";
    html << language_toggle_button("FR", "?lang=fr", switch_to_french.c_str(),
                                   language == Language::French);
    html << language_toggle_button("EN", "?lang=en", switch_to_english.c_str(),
                                   language == Language::English);
    html << "        </nav>\n";
    html << "        <h1 class=\"menu-header__title\">\n";
    html << "          <span class=\"menu-header__welcome\">" << gettext("Welcome") << "</span>\n";
    html << "          <span class=\"menu-header__connector\">" << gettext("to the")
         << "</span>\n";
    html << "          <span class=\"menu-header__brand\">" << gettext("BeaverKiosk") << "</span>\n";
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
