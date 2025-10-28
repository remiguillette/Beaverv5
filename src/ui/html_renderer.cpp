#include "ui/html_renderer.h"

#include <array>
#include <cctype>
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

struct ExtensionContact {
    const char* id;
    const char* name_fr;
    const char* name_en;
    const char* subtitle_fr;
    const char* subtitle_en;
    const char* details_fr;
    const char* details_en;
    const char* extension;
};

constexpr std::array<ExtensionContact, 4> kExtensionContacts = {{
    {"opp",
     "Police provinciale de l’Ontario",
     "Ontario Provincial Police",
     "Ligne interne",
     "Internal line",
     "Bureau 101",
     "Office 101",
     "1201"},
    {"spca",
     "SPCA Niagara",
     "SPCA Niagara",
     "Programme Paws Law",
     "Paws Law program",
     "Bureau 3434",
     "Office 3434",
     "3434"},
    {"mom",
     "Maman",
     "Mom",
     "Contact direct",
     "Direct line",
     "Bureau des plaintes",
     "Complaints Office",
     "22"},
    {"serviceOntario",
     "Services Ontario",
     "Services Ontario",
     "Gouvernement de l’Ontario",
     "Government of Ontario",
     "Poste *1345",
     "Desktop *1345",
     "1345"}
}};

std::string contact_initial(const ExtensionContact& contact, Language language) {
    const char* name = language == Language::French ? contact.name_fr : contact.name_en;
    if (!name || name[0] == '\0') {
        return "";
    }

    const unsigned char first = static_cast<unsigned char>(name[0]);
    const char initial = static_cast<char>(std::toupper(first));
    return std::string(1, initial);
}
}  // namespace

namespace {

std::string resolve_route(const AppTile& app, MenuRouteMode route_mode) {
    switch (route_mode) {
        case MenuRouteMode::kKiosk:
            return app.routes.kiosk;
        case MenuRouteMode::kHttpServer:
        default:
            return app.routes.http;
    }
}

}  // namespace

std::string generate_app_tile_html(const AppTile& app, const TranslationCatalog& translations,
                                   Language language, MenuRouteMode route_mode,
                                   const std::string& asset_prefix) {
    std::ostringstream html;

    const std::string route = resolve_route(app, route_mode);
    const bool has_route = !route.empty();
    if (has_route) {
        html << "<a href=\"" << route << "\" class=\"app-tile app-tile--" << app.accent
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
                                    MenuRouteMode route_mode,
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
        html << "        "
             << generate_app_tile_html(app, translations, language, route_mode, asset_prefix);
    }
    
    html << "      </main>\n";
    html << "    </div>\n";
    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}

namespace {

std::string build_menu_href(Language language, BeaverphoneMenuLinkMode menu_link_mode) {
    const char* lang_code = language == Language::French ? "fr" : "en";
    switch (menu_link_mode) {
        case BeaverphoneMenuLinkMode::kRelativeIndex:
            return std::string("index.html?lang=") + lang_code;
        case BeaverphoneMenuLinkMode::kAbsoluteRoot:
        default:
            return std::string("/?lang=") + lang_code;
    }
}

}  // namespace

std::string generate_beaverphone_dialpad_html(const TranslationCatalog& translations,
                                              Language language,
                                              const std::string& asset_prefix,
                                              BeaverphoneMenuLinkMode menu_link_mode) {
    std::ostringstream html;

    const char* lang_code = html_lang_code(language);
    const std::string beaverphone_label = translations.translate("BeaverPhone", language);
    const std::string dialpad_label = translations.translate("Dialpad", language);
    const std::string enter_number = translations.translate("Enter a number", language);
    const std::string call_label = translations.translate("Call", language);
    const std::string description = translations.translate("BeaverPhone description", language);
    const std::string extensions_title = translations.translate("Phone extensions", language);
    const std::string extension_prefix = translations.translate("Extension prefix", language);
    const std::string back_to_menu = translations.translate("Back to menu", language);
    const std::string language_label = translations.translate("Language selection", language);
    const std::string switch_to_french = translations.translate("Switch to French", language);
    const std::string switch_to_english = translations.translate("Switch to English", language);

    const std::string menu_href = build_menu_href(language, menu_link_mode);
    const bool use_absolute_beaverphone_links =
        (menu_link_mode == BeaverphoneMenuLinkMode::kAbsoluteRoot);
    const std::string beaverphone_base =
        use_absolute_beaverphone_links ? "/apps/beaverphone" : "apps/beaverphone";
    const std::string beaverphone_french_href = beaverphone_base + "?lang=fr";
    const std::string beaverphone_english_href = beaverphone_base + "?lang=en";

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
    html << "        <div class=\"phone-header__accent\" aria-hidden=\"true\"></div>\n";
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
    html << "          <h2 class=\"extensions-title\">" << extensions_title << "</h2>\n";
    html << "          <div class=\"extension-list\">\n";

    for (const auto& contact : kExtensionContacts) {
        const std::string name = language == Language::French ? contact.name_fr : contact.name_en;
        const std::string subtitle =
            language == Language::French ? contact.subtitle_fr : contact.subtitle_en;
        const std::string details =
            language == Language::French ? contact.details_fr : contact.details_en;

        html << "            <article class=\"extension-card\" data-extension-id=\"" << contact.id
             << "\">\n";
        const std::string initial = contact_initial(contact, language);
        html << "              <span class=\"extension-card__avatar\" aria-hidden=\"true\">" << initial
             << "</span>\n";
        html << "              <div class=\"extension-card__content\">\n";
        html << "                <h3 class=\"extension-card__name\">" << name << "</h3>\n";
        html << "                <p class=\"extension-card__subtitle\">" << subtitle << "</p>\n";
        html << "                <p class=\"extension-card__details\">" << details << "</p>\n";
        html << "              </div>\n";
        html << "              <div class=\"extension-card__extension\">\n";
        html << "                <span class=\"extension-card__extension-label\">" << extension_prefix
             << "</span>\n";
        html << "                <span class=\"extension-card__extension-value\">" << contact.extension
             << "</span>\n";
        html << "              </div>\n";
        html << "            </article>\n";
    }

    html << "          </div>\n";
    html << "        </aside>\n";
    html << "      </main>\n";
    html << "    </div>\n";
    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}
