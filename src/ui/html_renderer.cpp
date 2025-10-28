#include "ui/html_renderer.h"

#include <array>
#include <sstream>

#include "core/translation_catalog.h"

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

std::string language_toggle_button(const std::string& label, const std::string& href,
                                   const std::string& aria_label, bool active) {
    std::ostringstream html;
    html << "          <a href=\"" << href << "\" class=\"lang-toggle__button";
    if (active) {
        html << " lang-toggle__button--active";
    }
    html << "\" aria-pressed=\"" << (active ? "true" : "false") << "\" title=\"" << aria_label
         << "\">" << label << "</a>\n";
    return html.str();
}

std::string resolve_asset_path(const std::string& asset_prefix, const std::string& relative_path) {
    if (relative_path.empty()) {
        return relative_path;
    }

    if (asset_prefix.empty()) {
        return relative_path;
    }

    if (asset_prefix.back() == '/') {
        if (!relative_path.empty() && relative_path.front() == '/') {
            return asset_prefix + relative_path.substr(1);
        }
        return asset_prefix + relative_path;
    }

    if (!relative_path.empty() && relative_path.front() == '/') {
        return asset_prefix + relative_path;
    }

    return asset_prefix + "/" + relative_path;
}

struct DialpadKey {
    const char* symbol;
    const char* letters;
};

constexpr std::array<DialpadKey, 12> kDialpadKeys = {{{"1", ""},
                                                       {"2", "ABC"},
                                                       {"3", "DEF"},
                                                       {"4", "GHI"},
                                                       {"5", "JKL"},
                                                       {"6", "MNO"},
                                                       {"7", "PQRS"},
                                                       {"8", "TUV"},
                                                       {"9", "WXYZ"},
                                                       {"*", ""},
                                                       {"0", "+"},
                                                       {"#", ""}}};
}  // namespace

std::string generate_app_tile_html(const AppTile& app, const TranslationCatalog& translations,
                                   Language language, const std::string& asset_prefix) {
    std::ostringstream html;

    const bool has_route = !app.route.empty();
    if (has_route) {
        html << "<a href=\"" << app.route << "\" class=\"app-tile app-tile--" << app.accent
             << "\">\n";
    } else {
        html << "<button type=\"button\" class=\"app-tile app-tile--" << app.accent << "\">\n";
    }
    html << "  <div class=\"app-tile__icon\" aria-hidden=\"true\">\n";
    html << "    <img src=\"" << resolve_asset_path(asset_prefix, app.icon)
         << "\" alt=\"\" class=\"app-tile__icon-image\" loading=\"lazy\" />\n";
    html << "  </div>\n";
    html << "  <h3 class=\"app-tile__name\">"
         << translations.translate(app.name, language) << "</h3>\n";
    if (has_route) {
        html << "</a>\n";
    } else {
        html << "</button>\n";
    }

    return html.str();
}

std::string generate_menu_page_html(const std::vector<AppTile>& apps,
                                    const TranslationCatalog& translations, Language language,
                                    const std::string& asset_prefix) {
    std::ostringstream html;

    const char* lang_code = html_lang_code(language);
    const std::string language_label = translations.translate("Language selection", language);
    const std::string switch_to_french = translations.translate("Switch to French", language);
    const std::string switch_to_english = translations.translate("Switch to English", language);

    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"" << lang_code << "\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\" />\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";
    html << "  <title>BeaverKiosk - C++ Edition</title>\n";
    html << "  <link rel=\"preconnect\" href=\"https://fonts.googleapis.com\" />\n";
    html << "  <link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin />\n";
    html << "  <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap\" rel=\"stylesheet\" />\n";
    html << "  <link rel=\"stylesheet\" href=\""
         << resolve_asset_path(asset_prefix, "css/styles.css") << "\" />\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div id=\"root\">\n";
    html << "    <div class=\"menu-root\">\n";
    html << "      <header class=\"menu-header\">\n";
    html << "        <h1 class=\"menu-header__title\">\n";
    html << "          <span class=\"menu-header__welcome\">"
         << translations.translate("Welcome", language) << "</span>\n";
    html << "          <span class=\"menu-header__connector\">"
         << translations.translate("to the", language) << "</span>\n";
    html << "          <span class=\"menu-header__brand\">"
         << translations.translate("BeaverKiosk", language) << "</span>\n";
    html << "        </h1>\n";
    html << "        <nav class=\"lang-toggle\" role=\"group\" aria-label=\"" << language_label
         << "\">\n";
    html << language_toggle_button("FR", "?lang=fr", switch_to_french,
                                   language == Language::French);
    html << language_toggle_button("EN", "?lang=en", switch_to_english,
                                   language == Language::English);
    html << "        </nav>\n";
    html << "      </header>\n";
    html << "      <main class=\"menu-grid\">\n";

    for (const auto& app : apps) {
        html << "        " << generate_app_tile_html(app, translations, language, asset_prefix);
    }
    
    html << "      </main>\n";
    html << "    </div>\n";
    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}

std::string generate_beaverphone_dialpad_html(const TranslationCatalog& translations,
                                              Language language,
                                              const std::string& asset_prefix) {
    std::ostringstream html;

    const char* lang_code = html_lang_code(language);
    const std::string beaverphone_label = translations.translate("BeaverPhone", language);
    const std::string dialpad_label = translations.translate("Dialpad", language);
    const std::string enter_number = translations.translate("Enter a number", language);
    const std::string call_label = translations.translate("Call", language);
    const std::string description = translations.translate("BeaverPhone description", language);
    const std::string back_to_menu = translations.translate("Back to menu", language);
    const std::string language_label = translations.translate("Language selection", language);
    const std::string switch_to_french = translations.translate("Switch to French", language);
    const std::string switch_to_english = translations.translate("Switch to English", language);

    const std::string menu_href = language == Language::French ? "?lang=fr" : "?lang=en";
    const std::string beaverphone_french_href = "apps/beaverphone?lang=fr";
    const std::string beaverphone_english_href = "apps/beaverphone?lang=en";

    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"" << lang_code << "\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\" />\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";
    html << "  <title>" << beaverphone_label << " - BeaverKiosk</title>\n";
    html << "  <link rel=\"preconnect\" href=\"https://fonts.googleapis.com\" />\n";
    html << "  <link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin />\n";
    html << "  <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap\" rel=\"stylesheet\" />\n";
    html << "  <link rel=\"stylesheet\" href=\""
         << resolve_asset_path(asset_prefix, "css/styles.css") << "\" />\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div id=\"root\">\n";
    html << "    <div class=\"phone-page\">\n";
    html << "      <header class=\"phone-header\">\n";
    html << "        <a class=\"phone-back-link\" href=\"" << menu_href << "\">" << back_to_menu
         << "</a>\n";
    html << "        <h1 class=\"phone-title\">" << beaverphone_label << "</h1>\n";
    html << "        <nav class=\"lang-toggle\" role=\"group\" aria-label=\"" << language_label
         << "\">\n";
    html << language_toggle_button("FR", beaverphone_french_href, switch_to_french,
                                   language == Language::French);
    html << language_toggle_button("EN", beaverphone_english_href, switch_to_english,
                                   language == Language::English);
    html << "        </nav>\n";
    html << "      </header>\n";
    html << "      <main class=\"phone-main\">\n";
    html << "        <section class=\"dialpad-card\" aria-labelledby=\"dialpad-title\">\n";
    html << "          <h2 id=\"dialpad-title\" class=\"dialpad-title\">" << dialpad_label
         << "</h2>\n";
    html << "          <div class=\"dialpad-display\" aria-live=\"polite\" aria-atomic=\"true\">\n";
    html << "            <span class=\"dialpad-display__placeholder\">" << enter_number
         << "</span>\n";
    html << "          </div>\n";
    html << "          <div class=\"dialpad-grid\" role=\"group\" aria-label=\"" << dialpad_label
         << "\">\n";

    for (const auto& key : kDialpadKeys) {
        html << "            <button type=\"button\" class=\"dialpad-key\">\n";
        html << "              <span class=\"dialpad-key__symbol\">" << key.symbol
             << "</span>\n";
        html << "              <span class=\"dialpad-key__letters\">";
        if (key.letters[0] != '\0') {
            html << key.letters;
        } else {
            html << "&nbsp;";
        }
        html << "</span>\n";
        html << "            </button>\n";
    }

    html << "          </div>\n";
    html << "          <button type=\"button\" class=\"dialpad-call-button\">" << call_label
         << "</button>\n";
    html << "        </section>\n";
    html << "        <aside class=\"dialpad-details\">\n";
    html << "          <p class=\"dialpad-description\">" << description << "</p>\n";
    html << "        </aside>\n";
    html << "      </main>\n";
    html << "    </div>\n";
    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}
