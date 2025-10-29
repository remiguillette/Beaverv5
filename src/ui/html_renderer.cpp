#include "ui/html_renderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

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

std::string html_escape(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '\"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#39;";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }
    return escaped;
}

std::string json_escape(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (unsigned char ch : text) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '\"':
                escaped += "\\\"";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (ch < 0x20) {
                    std::ostringstream hex_stream;
                    hex_stream << "\\u" << std::uppercase << std::setfill('0') << std::setw(4)
                               << static_cast<int>(ch);
                    escaped += hex_stream.str();
                } else {
                    escaped.push_back(static_cast<char>(ch));
                }
                break;
        }
    }
    return escaped;
}

std::string json_array(const std::vector<std::string>& values) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        out << '\"' << json_escape(values[i]) << '\"';
        if (i + 1 < values.size()) {
            out << ", ";
        }
    }
    out << "]";
    return out.str();
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

struct AlarmKey {
    const char* symbol;
};

constexpr std::array<AlarmKey, 12> kAlarmKeys = {{{"1"},
                                                  {"2"},
                                                  {"3"},
                                                  {"4"},
                                                  {"5"},
                                                  {"6"},
                                                  {"7"},
                                                  {"8"},
                                                  {"9"},
                                                  {"*"},
                                                  {"0"},
                                                  {"#"}}};

struct AlarmIndicatorDefinition {
    const char* id;
    const char* badge_modifier;
    const char* translation_key;
};

constexpr std::array<AlarmIndicatorDefinition, 4> kAlarmIndicators = {{
    {"fire", "alarm-status__badge--fire", "Fire"},
    {"police", "alarm-status__badge--police", "Police"},
    {"medical", "alarm-status__badge--medical", "Medical"},
    {"auxiliary", "alarm-status__badge--auxiliary", "Auxiliary"},
}};

struct ExtensionContact {
    const char* id;
    const char* name_fr;
    const char* name_en;
    const char* subtitle_fr;
    const char* subtitle_en;
    const char* details_fr;
    const char* details_en;
    const char* extension;
    const char* icon_path;
};

constexpr std::array<ExtensionContact, 4> kExtensionContacts = {{
    {"opp",
     "Police provinciale de l’Ontario",
     "Ontario Provincial Police",
     "Ligne interne",
     "Internal line",
     "Bureau 101",
     "Office 101",
     "1201",
     "contact/Police.svg"},
    {"spca",
     "SPCA Niagara",
     "SPCA Niagara",
     "Programme Paws Law",
     "Paws Law program",
     "Bureau 3434",
     "Office 3434",
     "3434",
     "contact/SPCA.svg"},
    {"mom",
     "Maman",
     "Mom",
     "Contact direct",
     "Direct line",
     "Bureau des plaintes",
     "Complaints Office",
     "0022",
     "contact/mom.svg"},
    {"serviceOntario",
     "Services Ontario",
     "Services Ontario",
     "Gouvernement de l’Ontario",
     "Government of Ontario",
     "Poste *1345",
     "Desktop *1345",
     "1345",
     "contact/ontario.svg"}
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

const RouteEntry* resolve_route(const AppTile& app, MenuRouteMode route_mode) {
    switch (route_mode) {
        case MenuRouteMode::kKiosk:
            return &app.routes.kiosk;
        case MenuRouteMode::kHttpServer:
        default:
            return &app.routes.http;
    }
}

}  // namespace

std::string generate_app_tile_html(const AppTile& app, const TranslationCatalog& translations,
                                   Language language, MenuRouteMode route_mode,
                                   const std::string& asset_prefix) {
    std::ostringstream html;

    const RouteEntry* route_entry = resolve_route(app, route_mode);
    const std::string route = route_entry != nullptr ? route_entry->uri : "";
    const bool has_route = !route.empty();
    if (has_route) {
        html << "<a href=\"" << route << "\" class=\"app-tile app-tile--" << app.accent
             << "\"";
        if (route_entry != nullptr && route_entry->remote) {
            html << " data-remote=\"true\"";
        }
        html << ">\n";
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

std::string build_menu_href_common(Language language, bool use_relative_index) {
    const char* lang_code = language == Language::French ? "fr" : "en";
    if (use_relative_index) {
        return std::string("index.html?lang=") + lang_code;
    }
    return std::string("/?lang=") + lang_code;
}

std::string build_menu_href(Language language, BeaverphoneMenuLinkMode menu_link_mode) {
    const bool use_relative = (menu_link_mode == BeaverphoneMenuLinkMode::kRelativeIndex);
    return build_menu_href_common(language, use_relative);
}

std::string build_menu_href(Language language, BeaverSystemMenuLinkMode menu_link_mode) {
    const bool use_relative = (menu_link_mode == BeaverSystemMenuLinkMode::kRelativeIndex);
    return build_menu_href_common(language, use_relative);
}

std::string build_menu_href(Language language, BeaverAlarmMenuLinkMode menu_link_mode) {
    const bool use_relative = (menu_link_mode == BeaverAlarmMenuLinkMode::kRelativeIndex);
    return build_menu_href_common(language, use_relative);
}

std::string build_menu_href(Language language, BeaverTaskMenuLinkMode menu_link_mode) {
    const bool use_relative = (menu_link_mode == BeaverTaskMenuLinkMode::kRelativeIndex);
    return build_menu_href_common(language, use_relative);
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
    const std::string clear_label = translations.translate("Clear", language);
    const std::string extensions_title = translations.translate("Phone extensions", language);
    const std::string extension_prefix = translations.translate("Extension prefix", language);
    const std::string connection_connected = translations.translate("Connected", language);
    const std::string connection_disconnected =
        translations.translate("Not connected", language);
    const std::string connection_connecting =
        translations.translate("Connection in progress", language);
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
    html << "          <div class=\"dialpad-title-bar\">\n";
    html << "            <h2 id=\"dialpad-title\" class=\"dialpad-title\">" << dialpad_label
         << "</h2>\n";
    html << "            <div class=\"connection-indicator\" role=\"status\" aria-live=\"polite\""
         << " data-status=\"disconnected\""
         << " data-label-connected=\"" << connection_connected << "\""
         << " data-label-connecting=\"" << connection_connecting << "\""
         << " data-label-disconnected=\"" << connection_disconnected << "\">\n";
    html << "              <span class=\"connection-indicator__dot\" aria-hidden=\"true\"></span>\n";
    html << "              <span class=\"connection-indicator__label\">" << connection_disconnected
         << "</span>\n";
    html << "            </div>\n";
    html << "          </div>\n";
    html << "          <div class=\"dialpad-display is-empty\" aria-live=\"polite\" aria-atomic=\"true\""
         << " data-placeholder=\"" << enter_number << "\">\n";
    html << "            <span class=\"dialpad-display__value\">" << enter_number << "</span>\n";
    html << "          </div>\n";
    html << "          <div class=\"dialpad-grid\" role=\"group\" aria-label=\"" << dialpad_label
         << "\">\n";

    for (const auto& key : kDialpadKeys) {
        html << "            <button type=\"button\" class=\"dialpad-key\"";
        if (key.symbol[0] != '\0' && std::isdigit(static_cast<unsigned char>(key.symbol[0]))) {
            html << " data-digit=\"" << key.symbol << "\"";
        }
        html << ">\n";
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
    html << "          <div class=\"dialpad-actions\">\n";
    html << "            <button type=\"button\" class=\"dialpad-action dialpad-action--clear\""
         << " data-action=\"clear\">" << clear_label << "</button>\n";
    html << "            <button type=\"button\" class=\"dialpad-call-button\""
         << " data-action=\"call\" aria-label=\"" << call_label << "\">\n";
    html << "              <span class=\"dialpad-call-button__icon\" aria-hidden=\"true\">\n";
    html << "                <svg viewBox=\"0 0 24 24\" focusable=\"false\" aria-hidden=\"true\">\n";
    html << "                  <path d=\"M6.62 10.79a15.053 15.053 0 006.59 6.59l2.2-2.2a1 1 0 011.01-.24 11.05"
         << " 11.05 0 003.46.55 1 1 0 011 1V20a1 1 0 01-1 1 16 16 0 01-16-16 1 1 0 011-1h3.5a1 1 0 011 1"
         << " 11.05 11.05 0 00.55 3.46 1 1 0 01-.24 1.01l-2.2 2.2z\" fill=\"currentColor\"/>\n";
    html << "                </svg>\n";
    html << "            </span>\n";
    html << "            </button>\n";
    html << "          </div>\n";
    html << "        </section>\n";
    html << "        <aside class=\"dialpad-details\">\n";
    html << "          <h2 class=\"extensions-title\">" << extensions_title << "</h2>\n";
    html << "          <div class=\"extension-list\">\n";

    for (const auto& contact : kExtensionContacts) {
        const std::string name = language == Language::French ? contact.name_fr : contact.name_en;
        const std::string subtitle =
            language == Language::French ? contact.subtitle_fr : contact.subtitle_en;
        const std::string details =
            language == Language::French ? contact.details_fr : contact.details_en;

        html << "            <article class=\"extension-card\" data-extension-id=\"" << contact.id
             << "\" data-extension-value=\"" << contact.extension
             << "\">\n";
        const std::string initial = contact_initial(contact, language);
        const std::string icon_path = contact.icon_path ? contact.icon_path : "";
        if (!icon_path.empty()) {
            html << "              <span class=\"extension-card__avatar extension-card__avatar--has-image\""
                 << " aria-hidden=\"true\">\n";
            html << "                <img src=\"" << resolve_asset_path(asset_prefix, icon_path)
                 << "\" alt=\"\" class=\"extension-card__avatar-image\" loading=\"lazy\" />\n";
            html << "              </span>\n";
        } else {
            html << "              <span class=\"extension-card__avatar\" aria-hidden=\"true\">" << initial
                 << "</span>\n";
        }
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
    html << R"(    <script>
      (function() {
        const doc = document;
        const displayWrapper = doc.querySelector('.dialpad-display');
        const displayValue = doc.querySelector('.dialpad-display__value');
        const placeholder = displayWrapper ? displayWrapper.getAttribute('data-placeholder') || '' : '';
        const callButton = doc.querySelector('.dialpad-call-button');
        const clearButton = doc.querySelector('.dialpad-action--clear');
        const dialpad = doc.querySelector('.dialpad-grid');
        const extensions = doc.querySelector('.extension-list');
        const connectionIndicator = doc.querySelector('.connection-indicator');
        const connectionLabel = connectionIndicator
          ? connectionIndicator.querySelector('.connection-indicator__label')
          : null;
        const connectionLabels = connectionIndicator
          ? {
              connected: connectionIndicator.getAttribute('data-label-connected') || 'Connected',
              connecting: connectionIndicator.getAttribute('data-label-connecting') || 'Connecting…',
              disconnected:
                connectionIndicator.getAttribute('data-label-disconnected') || 'Disconnected',
            }
          : null;
        const wsScheme = window.location.protocol === 'https:' ? 'wss' : 'ws';
        const wsHost = window.location.hostname || '192.168.1.60';
        const wsUrl = `${wsScheme}://${wsHost}:5001`;
        console.info('[BeaverPhone] WebSocket endpoint:', wsUrl);
        const reconnectDelayMs = 5000;
        let socket = null;
        let reconnectTimer = 0;
        let shouldReconnect = true;
        const raf = window.requestAnimationFrame ? window.requestAnimationFrame.bind(window)
                                                 : (cb) => window.setTimeout(cb, 16);
        const digits = [];
        const maxPhoneLength = 10;
        const maxExtensionLength = 4;
        let pendingFrame = false;
        let lastRendered = '';
        let isDisplayEmpty = !displayWrapper || displayWrapper.classList.contains('is-empty');
        let lastCallButtonDisabled = callButton ? callButton.disabled : true;

        const isCompleteLength = (length) => length === maxPhoneLength || length === maxExtensionLength;

        const formatDigits = (buffer) => {
          if (buffer.length <= maxExtensionLength) {
            return buffer.join('');
          }
          const joined = buffer.join('');
          const area = joined.slice(0, 3);
          const central = joined.slice(3, 6);
          const line = joined.slice(6, maxPhoneLength);
          if (joined.length <= 6) {
            return `(${area})-${central}`;
          }
          return `(${area})-${central}-${line}`;
        };

        const render = () => {
          pendingFrame = false;
          if (!displayWrapper || !displayValue) {
            return;
          }

          const shouldBeEmpty = digits.length === 0;
          if (shouldBeEmpty) {
            if (!isDisplayEmpty) {
              displayWrapper.classList.add('is-empty');
              isDisplayEmpty = true;
            }
            if (lastRendered !== placeholder) {
              displayValue.textContent = placeholder;
              lastRendered = placeholder;
            }
          } else {
            if (isDisplayEmpty) {
              displayWrapper.classList.remove('is-empty');
              isDisplayEmpty = false;
            }
            const joined = formatDigits(digits);
            if (joined !== lastRendered) {
              displayValue.textContent = joined;
              lastRendered = joined;
            }
          }

          if (callButton) {
            const shouldDisable = !isCompleteLength(digits.length);
            if (shouldDisable !== lastCallButtonDisabled) {
              callButton.disabled = shouldDisable;
              lastCallButtonDisabled = shouldDisable;
            }
          }
        };

        const scheduleRender = () => {
          if (pendingFrame) {
            return;
          }
          pendingFrame = true;
          raf(render);
        };

        const appendDigit = (digit) => {
          if (!digit || digits.length >= maxPhoneLength) {
            return;
          }
          digits.push(digit);
          scheduleRender();
        };

        const clearDigits = () => {
          if (digits.length) {
            digits.length = 0;
          }
          scheduleRender();
        };

        const setDigits = (value, isExtension = false) => {
          const sanitized = (value || '').replace(/\D/g, '');
          const limit = isExtension ? maxExtensionLength : maxPhoneLength;
          digits.length = 0;
          for (let i = 0; i < sanitized.length && i < limit; ++i) {
            digits.push(sanitized[i]);
          }
          scheduleRender();
        };

        const clearReconnectTimer = () => {
          if (reconnectTimer) {
            window.clearTimeout(reconnectTimer);
            reconnectTimer = 0;
          }
        };

        const setConnectionStatus = (status) => {
          if (!connectionIndicator || !connectionLabels || !connectionLabel) {
            return;
          }
          if (connectionIndicator.getAttribute('data-status') !== status) {
            connectionIndicator.setAttribute('data-status', status);
          }
          const nextLabel = connectionLabels[status] || '';
          if (connectionLabel.textContent !== nextLabel) {
            connectionLabel.textContent = nextLabel;
          }
        };

        const scheduleReconnect = () => {
          if (!shouldReconnect) {
            console.info(
              '[BeaverPhone] WebSocket reconnect suppressed because shouldReconnect=false.'
            );
            return;
          }
          clearReconnectTimer();
          console.info(
            '[BeaverPhone] Scheduling WebSocket reconnect in',
            reconnectDelayMs,
            'ms.'
          );
          reconnectTimer = window.setTimeout(() => {
            console.info('[BeaverPhone] Attempting WebSocket reconnect now.');
            setConnectionStatus('connecting');
            connectSocket();
          }, reconnectDelayMs);
        };

        const connectSocket = () => {
          clearReconnectTimer();
          if (
            socket &&
            socket.readyState !== WebSocket.CLOSED &&
            socket.readyState !== WebSocket.CLOSING
          ) {
            try {
              console.info(
                '[BeaverPhone] Closing existing WebSocket before establishing a new one.',
                { readyState: socket.readyState }
              );
              socket.close();
            } catch (error) {
              console.warn('[BeaverPhone] Unable to close previous WebSocket instance.', error);
            }
          }

          let nextSocket;
          try {
            console.info('[BeaverPhone] Attempting to open WebSocket connection...');
            nextSocket = new WebSocket(wsUrl);
          } catch (error) {
            console.error('[BeaverPhone] Failed to create WebSocket connection.', error);
            scheduleReconnect();
            return;
          }

          socket = nextSocket;
          setConnectionStatus('connecting');

          socket.addEventListener('open', () => {
            console.info('[BeaverPhone] WebSocket connection established successfully.');
            setConnectionStatus('connected');
          });

          socket.addEventListener('message', (event) => {
            console.debug('[BeaverPhone] Message received from WS server:', event.data);
          });

          socket.addEventListener('close', (event) => {
            console.warn(
              '[BeaverPhone] WebSocket closed.',
              {
                code: event.code,
                reason: event.reason,
                wasClean: event.wasClean,
              }
            );
            setConnectionStatus('disconnected');
            scheduleReconnect();
          });

          socket.addEventListener('error', (event) => {
            console.error('[BeaverPhone] WebSocket error event received.', event);
            if (socket === nextSocket) {
              try {
                socket.close();
              } catch (closeError) {
                console.warn('[BeaverPhone] Error while closing WebSocket after failure.', closeError);
              }
            }
          });
        };

        const resetAfterCall = () => {
          clearDigits();
          if (callButton) {
            callButton.blur();
          }
        };

        const sendDialPayload = (dialDigits) => {
          if (!dialDigits) {
            return false;
          }
          if (!socket || socket.readyState !== WebSocket.OPEN) {
            console.warn('[BeaverPhone] WebSocket is not connected. Payload not sent.');
            return false;
          }
          try {
            const payload = {
              type: 'dial',
              action: 'dial',
              number: dialDigits,
              source: 'BeaverPhone Dialpad',
            };
            socket.send(JSON.stringify(payload));
            console.debug('[BeaverPhone] Dial payload sent.', payload);
            resetAfterCall();
            return true;
          } catch (error) {
            console.error('[BeaverPhone] Failed to send dial payload.', error);
            return false;
          }
        };

        window.addEventListener('beforeunload', () => {
          shouldReconnect = false;
          clearReconnectTimer();
          if (socket && socket.readyState === WebSocket.OPEN) {
            try {
              socket.close();
            } catch (error) {
              console.warn('[BeaverPhone] Error while closing WebSocket on unload.', error);
            }
          }
        });

        window.addEventListener('beaverphone:call', (event) => {
          if (!event || !event.detail) {
            return;
          }
          const dialDigits = event.detail.digits || '';
          sendDialPayload(dialDigits);
        });

        connectSocket();

        if (dialpad) {
          dialpad.addEventListener('click', (event) => {
            const button = event.target.closest('.dialpad-key');
            if (!button || !dialpad.contains(button)) {
              return;
            }
            const digit = button.getAttribute('data-digit');
            if (!digit) {
              return;
            }
            appendDigit(digit);
          }, { passive: true });
        }

        if (extensions) {
          extensions.addEventListener('click', (event) => {
            const card = event.target.closest('.extension-card');
            if (!card || !extensions.contains(card)) {
              return;
            }
            const extension = card.getAttribute('data-extension-value');
            setDigits(extension, true);
          }, { passive: true });
        }

        if (clearButton) {
          clearButton.addEventListener('click', () => {
            clearDigits();
            clearButton.blur();
          }, { passive: true });
        }

        if (callButton) {
          callButton.addEventListener('click', () => {
            if (callButton.disabled) {
              return;
            }
            const payload = digits.join('');
            const dispatchCall = () => {
              window.dispatchEvent(
                new CustomEvent('beaverphone:call', { detail: { digits: payload } })
              );
            };
            if (typeof queueMicrotask === 'function') {
              queueMicrotask(dispatchCall);
            } else {
              window.setTimeout(dispatchCall, 0);
            }
          }, { passive: true });
        }

        render();
      })();
    </script>
)";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}

std::string generate_beaveralarm_console_html(const TranslationCatalog& translations,
                                              Language language,
                                              const std::string& asset_prefix,
                                              BeaverAlarmMenuLinkMode menu_link_mode) {
    std::ostringstream html;

    const char* lang_code = html_lang_code(language);
    const std::string alarm_label = translations.translate("BeaverAlarm", language);
    const std::string keypad_label = translations.translate("Alarm keypad", language);
    const std::string enter_code_label = translations.translate("Enter code", language);
    const std::string arm_label = translations.translate("Arm", language);
    const std::string disarm_label = translations.translate("Disarm", language);
    const std::string panic_label = translations.translate("Panic", language);
    const std::string clear_label = translations.translate("Clear", language);
    const std::string status_title = translations.translate("Status indicators", language);
    const std::string camera_title = translations.translate("Live webcam", language);
    const std::string camera_subtitle = translations.translate("Activate webcam", language);
    const std::string camera_ready_label = translations.translate("Allow camera access to start live feed.", language);
    const std::string camera_active_label = translations.translate("Camera active", language);
    const std::string camera_error_label = translations.translate("Unable to access webcam", language);
    const std::string camera_start_label = translations.translate("Start feed", language);
    const std::string camera_stop_label = translations.translate("Stop feed", language);
    const std::string ready_label = translations.translate("System ready", language);
    const std::string armed_label = translations.translate("Alarm armed", language);
    const std::string disarmed_label = translations.translate("System disarmed", language);
    const std::string alert_label = translations.translate("Alarm triggered", language);
    const std::string online_label = translations.translate("Online", language);
    const std::string offline_label = translations.translate("Offline", language);
    const std::string alert_status_label = translations.translate("Alert", language);
    const std::string back_to_menu = translations.translate("Back to menu", language);
    const std::string language_label = translations.translate("Language selection", language);
    const std::string switch_to_french = translations.translate("Switch to French", language);
    const std::string switch_to_english = translations.translate("Switch to English", language);

    const std::string menu_href = build_menu_href(language, menu_link_mode);
    const bool use_absolute_alarm_links =
        (menu_link_mode == BeaverAlarmMenuLinkMode::kAbsoluteRoot);
    const std::string alarm_base =
        use_absolute_alarm_links ? "/apps/beaveralarm" : "apps/beaveralarm";
    const std::string alarm_french_href = alarm_base + "?lang=fr";
    const std::string alarm_english_href = alarm_base + "?lang=en";

    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"" << lang_code << "\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\" />\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";
    html << "  <title>" << html_escape(alarm_label) << " - BeaverKiosk</title>\n";
    html << "  <link rel=\"stylesheet\" href=\""
         << resolve_asset_path(asset_prefix, "css/styles.css") << "\" />\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div id=\"root\">\n";
    html << "    <div class=\"alarm-page\">\n";
    html << "      <header class=\"alarm-header\">\n";
    html << "        <a class=\"alarm-back-link\" href=\"" << menu_href << "\">"
         << html_escape(back_to_menu) << "</a>\n";
    html << "        <h1 class=\"alarm-title\">" << html_escape(alarm_label) << "</h1>\n";
    html << "        <nav class=\"lang-toggle\" role=\"group\" aria-label=\""
         << html_escape(language_label) << "\">\n";
    html << language_toggle_button("FR", alarm_french_href, switch_to_french,
                                   language == Language::French);
    html << language_toggle_button("EN", alarm_english_href, switch_to_english,
                                   language == Language::English);
    html << "        </nav>\n";
    html << "        <div class=\"alarm-header__accent\" aria-hidden=\"true\"></div>\n";
    html << "      </header>\n";
    html << "      <main class=\"alarm-layout\">\n";
    html << "        <section class=\"alarm-card alarm-card--keypad\" aria-labelledby=\"alarm-keypad-title\">\n";
    html << "          <div class=\"alarm-card__header\">\n";
    html << "            <h2 id=\"alarm-keypad-title\" class=\"alarm-card__title\">"
         << html_escape(keypad_label) << "</h2>\n";
    html << "            <p class=\"alarm-card__subtitle\" data-role=\"alarm-subtitle\""
         << " data-label-ready=\"" << html_escape(ready_label) << "\""
         << " data-label-armed=\"" << html_escape(armed_label) << "\""
         << " data-label-disarmed=\"" << html_escape(disarmed_label) << "\""
         << " data-label-alert=\"" << html_escape(alert_label) << "\">"
         << html_escape(ready_label) << "</p>\n";
    html << "          </div>\n";
    html << "          <div class=\"alarm-display is-empty\" aria-live=\"polite\" aria-atomic=\"true\""
         << " data-placeholder=\"" << html_escape(enter_code_label) << "\">\n";
    html << "            <span class=\"alarm-display__value\">" << html_escape(enter_code_label)
         << "</span>\n";
    html << "          </div>\n";
    html << "          <div class=\"alarm-keypad\" role=\"group\" aria-label=\""
         << html_escape(keypad_label) << "\">\n";

    for (const auto& key : kAlarmKeys) {
        html << "            <button type=\"button\" class=\"alarm-key\" data-key=\""
             << html_escape(key.symbol) << "\">" << html_escape(key.symbol)
             << "</button>\n";
    }

    html << "          </div>\n";
    html << "          <div class=\"alarm-keypad__actions\">\n";
    html << "            <button type=\"button\" class=\"alarm-action alarm-action--clear\""
         << " data-action=\"clear\">" << html_escape(clear_label) << "</button>\n";
    html << "            <button type=\"button\" class=\"alarm-action alarm-action--arm\""
         << " data-action=\"arm\">" << html_escape(arm_label) << "</button>\n";
    html << "            <button type=\"button\" class=\"alarm-action alarm-action--disarm\""
         << " data-action=\"disarm\">" << html_escape(disarm_label) << "</button>\n";
    html << "            <button type=\"button\" class=\"alarm-action alarm-action--panic\""
         << " data-action=\"panic\">" << html_escape(panic_label) << "</button>\n";
    html << "          </div>\n";
    html << "        </section>\n";
    html << "        <section class=\"alarm-card alarm-card--camera\" aria-labelledby=\"alarm-camera-title\">\n";
    html << "          <div class=\"alarm-card__header\">\n";
    html << "            <h2 id=\"alarm-camera-title\" class=\"alarm-card__title\">"
         << html_escape(camera_title) << "</h2>\n";
    html << "            <p class=\"alarm-card__subtitle\" data-role=\"camera-status\""
         << " data-label-idle=\"" << html_escape(camera_subtitle) << "\""
         << " data-label-active=\"" << html_escape(camera_active_label) << "\""
         << " data-label-error=\"" << html_escape(camera_error_label) << "\">"
         << html_escape(camera_subtitle) << "</p>\n";
    html << "          </div>\n";
    html << "          <div class=\"alarm-camera\">\n";
    html << "            <div class=\"alarm-camera__display\">\n";
    html << "              <video class=\"alarm-camera__video\" playsinline autoplay muted></video>\n";
    html << "              <div class=\"alarm-camera__overlay\" data-role=\"camera-overlay\""
         << " data-label-idle=\"" << html_escape(camera_ready_label) << "\""
         << " data-label-active=\"" << html_escape(camera_active_label) << "\""
         << " data-label-error=\"" << html_escape(camera_error_label) << "\">"
         << html_escape(camera_ready_label) << "</div>\n";
    html << "            </div>\n";
    html << "            <div class=\"alarm-camera__actions\">\n";
    html << "              <button type=\"button\" class=\"alarm-action alarm-action--camera-start\""
         << " data-action=\"camera-start\">" << html_escape(camera_start_label) << "</button>\n";
    html << "              <button type=\"button\" class=\"alarm-action alarm-action--camera-stop\""
         << " data-action=\"camera-stop\" disabled>" << html_escape(camera_stop_label)
         << "</button>\n";
    html << "            </div>\n";
    html << "          </div>\n";
    html << "        </section>\n";
    html << "        <section class=\"alarm-card alarm-card--status\" aria-labelledby=\"alarm-status-title\">\n";
    html << "          <div class=\"alarm-card__header\">\n";
    html << "            <h2 id=\"alarm-status-title\" class=\"alarm-card__title\">"
         << html_escape(status_title) << "</h2>\n";
    html << "            <p class=\"alarm-card__subtitle\" data-role=\"alarm-subtitle\""
         << " data-label-ready=\"" << html_escape(ready_label) << "\""
         << " data-label-armed=\"" << html_escape(armed_label) << "\""
         << " data-label-disarmed=\"" << html_escape(disarmed_label) << "\""
         << " data-label-alert=\"" << html_escape(alert_label) << "\">"
         << html_escape(ready_label) << "</p>\n";
    html << "          </div>\n";
    html << "          <ul class=\"alarm-status-list\">\n";

    for (const auto& indicator : kAlarmIndicators) {
        const std::string indicator_label =
            translations.translate(indicator.translation_key, language);
        html << "            <li class=\"alarm-status\" data-indicator=\"" << indicator.id
             << "\" data-state=\"online\">\n";
        html << "              <span class=\"alarm-status__badge " << indicator.badge_modifier
             << "\" aria-hidden=\"true\"></span>\n";
        html << "              <div class=\"alarm-status__content\">\n";
        html << "                <span class=\"alarm-status__label\">"
             << html_escape(indicator_label) << "</span>\n";
        html << "                <span class=\"alarm-status__value\" data-label-online=\""
             << html_escape(online_label) << "\" data-label-offline=\""
             << html_escape(offline_label) << "\" data-label-alert=\""
             << html_escape(alert_status_label) << "\">" << html_escape(online_label)
             << "</span>\n";
        html << "              </div>\n";
        html << "            </li>\n";
    }

    html << "          </ul>\n";
    html << "        </section>\n";
    html << "      </main>\n";
    html << "    </div>\n";
    html << "  </div>\n";
    html << "  <script>\n";
    html << "    (() => {\n";
    html << "      const page = document.querySelector('.alarm-page');\n";
    html << "      if (!page) {\n";
    html << "        return;\n";
    html << "      }\n";
    html << "      const display = page.querySelector('.alarm-display');\n";
    html << "      const displayValue = page.querySelector('.alarm-display__value');\n";
    html << "      const keypad = page.querySelector('.alarm-keypad');\n";
    html << "      const subtitleElements = Array.from(page.querySelectorAll('[data-role=\"alarm-subtitle\"]'));\n";
    html << "      const statusItems = Array.from(page.querySelectorAll('.alarm-status'));\n";
    html << "      const placeholder = display ? display.getAttribute('data-placeholder') || '' : '';\n";
    html << "      const maxLength = 6;\n";
    html << "      let digits = [];\n";
    html << "      let cameraStream = null;\n";
    html << "      const updateSubtitles = (stateKey) => {\n";
    html << "        subtitleElements.forEach((element) => {\n";
    html << "          const nextLabel = element.getAttribute('data-label-' + stateKey) || '';\n";
    html << "          if (nextLabel) {\n";
    html << "            element.textContent = nextLabel;\n";
    html << "          }\n";
    html << "        });\n";
    html << "      };\n";
    html << "      const renderDisplay = () => {\n";
    html << "        if (!display || !displayValue) {\n";
    html << "          return;\n";
    html << "        }\n";
    html << "        if (!digits.length) {\n";
    html << "          display.classList.add('is-empty');\n";
    html << "          displayValue.textContent = placeholder;\n";
    html << "        } else {\n";
    html << "          display.classList.remove('is-empty');\n";
    html << "          displayValue.textContent = digits.map(() => '•').join('');\n";
    html << "        }\n";
    html << "      };\n";
    html << "      const updateStatuses = (stateKey) => {\n";
    html << "        statusItems.forEach((item) => {\n";
    html << "          const value = item.querySelector('.alarm-status__value');\n";
    html << "          if (!value) {\n";
    html << "            return;\n";
    html << "          }\n";
    html << "          const label = value.getAttribute('data-label-' + stateKey) || value.textContent;\n";
    html << "          value.textContent = label;\n";
    html << "          item.setAttribute('data-state', stateKey);\n";
    html << "        });\n";
    html << "      };\n";
    html << "      const setMode = (mode) => {\n";
    html << "        page.setAttribute('data-alarm-mode', mode);\n";
    html << "        if (mode === 'armed') {\n";
    html << "          updateStatuses('online');\n";
    html << "          updateSubtitles('armed');\n";
    html << "        } else if (mode === 'disarmed') {\n";
    html << "          updateStatuses('offline');\n";
    html << "          updateSubtitles('disarmed');\n";
    html << "        } else if (mode === 'alert') {\n";
    html << "          updateStatuses('alert');\n";
    html << "          updateSubtitles('alert');\n";
    html << "        } else {\n";
    html << "          updateStatuses('online');\n";
    html << "          updateSubtitles('ready');\n";
    html << "        }\n";
    html << "      };\n";
    html << "      const cameraCard = page.querySelector('.alarm-card--camera');\n";
    html << "      const cameraStatus = cameraCard ? cameraCard.querySelector('[data-role=\"camera-status\"]') : null;\n";
    html << "      const cameraOverlay = cameraCard ? cameraCard.querySelector('[data-role=\"camera-overlay\"]') : null;\n";
    html << "      const cameraStart = cameraCard ? cameraCard.querySelector('[data-action=\"camera-start\"]') : null;\n";
    html << "      const cameraStop = cameraCard ? cameraCard.querySelector('[data-action=\"camera-stop\"]') : null;\n";
    html << "      const cameraVideo = cameraCard ? cameraCard.querySelector('video') : null;\n";
    html << "      const setCameraState = (state) => {\n";
    html << "        if (!cameraCard) {\n";
    html << "          return;\n";
    html << "        }\n";
    html << "        cameraCard.setAttribute('data-camera-state', state);\n";
    html << "        const applyLabel = (element) => {\n";
    html << "          if (!element) {\n";
    html << "            return;\n";
    html << "          }\n";
    html << "          const nextLabel = element.getAttribute('data-label-' + state);\n";
    html << "          if (nextLabel) {\n";
    html << "            element.textContent = nextLabel;\n";
    html << "          }\n";
    html << "        };\n";
    html << "        applyLabel(cameraStatus);\n";
    html << "        applyLabel(cameraOverlay);\n";
    html << "        if (cameraStart) {\n";
    html << "          cameraStart.disabled = state === 'active';\n";
    html << "        }\n";
    html << "        if (cameraStop) {\n";
    html << "          cameraStop.disabled = state !== 'active';\n";
    html << "        }\n";
    html << "      };\n";
    html << "      const stopCamera = () => {\n";
    html << "        if (!cameraStream) {\n";
    html << "          return;\n";
    html << "        }\n";
    html << "        cameraStream.getTracks().forEach((track) => track.stop());\n";
    html << "        cameraStream = null;\n";
    html << "        if (cameraVideo) {\n";
    html << "          cameraVideo.srcObject = null;\n";
    html << "        }\n";
    html << "        setCameraState('idle');\n";
    html << "      };\n";
    html << "      const startCamera = async () => {\n";
    html << "        if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {\n";
    html << "          setCameraState('error');\n";
    html << "          return;\n";
    html << "        }\n";
    html << "        try {\n";
    html << "          cameraStream = await navigator.mediaDevices.getUserMedia({ video: true, audio: false });\n";
    html << "          if (cameraVideo) {\n";
    html << "            cameraVideo.srcObject = cameraStream;\n";
    html << "          }\n";
    html << "          setCameraState('active');\n";
    html << "        } catch (error) {\n";
    html << "          stopCamera();\n";
    html << "          setCameraState('error');\n";
    html << "        }\n";
    html << "      };\n";
    html << "      if (cameraStart) {\n";
    html << "        cameraStart.addEventListener('click', () => {\n";
    html << "          startCamera();\n";
    html << "        });\n";
    html << "      }\n";
    html << "      if (cameraStop) {\n";
    html << "        cameraStop.addEventListener('click', () => {\n";
    html << "          stopCamera();\n";
    html << "        });\n";
    html << "      }\n";
    html << "      setCameraState('idle');\n";
    html << "      window.addEventListener('beforeunload', () => {\n";
    html << "        stopCamera();\n";
    html << "      });\n";
    html << "      if (keypad) {\n";
    html << "        keypad.addEventListener('click', (event) => {\n";
    html << "          const button = event.target.closest('button');\n";
    html << "          if (!button || !keypad.contains(button)) {\n";
    html << "            return;\n";
    html << "          }\n";
    html << "          const key = button.getAttribute('data-key');\n";
    html << "          if (!key || digits.length >= maxLength) {\n";
    html << "            return;\n";
    html << "          }\n";
    html << "          digits.push(key);\n";
    html << "          renderDisplay();\n";
    html << "        }, { passive: true });\n";
    html << "      }\n";
    html << "      page.addEventListener('click', (event) => {\n";
    html << "        const actionButton = event.target.closest('.alarm-action');\n";
    html << "        if (!actionButton || !page.contains(actionButton)) {\n";
    html << "          return;\n";
    html << "        }\n";
    html << "        const action = actionButton.getAttribute('data-action');\n";
    html << "        if (action === 'clear') {\n";
    html << "          digits.length = 0;\n";
    html << "          renderDisplay();\n";
    html << "          setMode('idle');\n";
    html << "        } else if (action === 'arm') {\n";
    html << "          setMode('armed');\n";
    html << "        } else if (action === 'disarm') {\n";
    html << "          setMode('disarmed');\n";
    html << "        } else if (action === 'panic') {\n";
    html << "          setMode('alert');\n";
    html << "        }\n";
    html << "      });\n";
    html << "    })();\n";
    html << "  </script>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}

std::string generate_beaversystem_dashboard_html(const TranslationCatalog& translations,
                                                 Language language,
                                                 const std::string& asset_prefix,
                                                 BeaverSystemMenuLinkMode menu_link_mode,
                                                 const SystemStatusSnapshot& snapshot) {
    std::ostringstream html;
    auto append = [&](const std::string& text) { html << text << "\n"; };

    const char* lang_code = html_lang_code(language);
    const std::string beaversystem_label = translations.translate("BeaverSystem", language);
    const std::string language_label = translations.translate("Language selection", language);
    const std::string switch_to_french = translations.translate("Switch to French", language);
    const std::string switch_to_english = translations.translate("Switch to English", language);
    const std::string back_to_menu = translations.translate("Back to menu", language);
    const std::string system_status_title = translations.translate("System status", language);
    const std::string resource_usage_title = translations.translate("Resource usage", language);
    const std::string home_wifi_label = translations.translate("Home Wi-Fi", language);
    const std::string status_label = translations.translate("Status", language);
    const std::string interface_label = translations.translate("Interface", language);
    const std::string websocket_server_label = translations.translate("WebSocket server", language);
    const std::string last_message_label = translations.translate("Last message", language);
    const std::string system_battery_label = translations.translate("System battery", language);
    const std::string charge_label = translations.translate("Charge", language);
    const std::string debian_uptime_label = translations.translate("Debian uptime", language);
    const std::string uptime_label = translations.translate("Uptime", language);
    const std::string boot_time_label = translations.translate("Boot time", language);
    const std::string load_label = translations.translate("Load", language);
    const std::string websocket_channel_label = translations.translate("WebSocket channel", language);
    const std::string raw_uptime_label = translations.translate("Raw uptime", language);
    const std::string network_ports_label = translations.translate("Network ports", language);
    const std::string list_open_ports_label = translations.translate("List of open ports", language);
    const std::string no_ports_label = translations.translate("No listening ports detected.", language);
    const std::string no_telemetry_label = translations.translate("No telemetry received yet.", language);
    const std::string unavailable_label = translations.translate("Unavailable", language);
    const std::string not_connected_label = translations.translate("Not connected", language);
    const std::string connected_label = translations.translate("Connected", language);
    const std::string updated_label = translations.translate("Updated", language);
    const std::string unknown_label = translations.translate("Unknown", language);
    const std::string charging_label = translations.translate("Charging", language);
    const std::string discharging_label = translations.translate("Discharging", language);
    const std::string full_label = translations.translate("Full", language);
    const std::string not_charging_label = translations.translate("Not charging", language);

    const std::string menu_href = build_menu_href(language, menu_link_mode);
    const bool use_absolute_links = (menu_link_mode == BeaverSystemMenuLinkMode::kAbsoluteRoot);
    const std::string beaversystem_base = use_absolute_links ? "/apps/beaversystem" : "apps/beaversystem";
    const std::string beaversystem_french_href = beaversystem_base + "?lang=fr";
    const std::string beaversystem_english_href = beaversystem_base + "?lang=en";

    const std::string last_updated_value = snapshot.generated_at_iso.empty() ? std::string("--") : snapshot.generated_at_iso;
    const std::string debian_uptime_value = snapshot.debian.uptime_human.empty() ? unknown_label : snapshot.debian.uptime_human;
    const std::string debian_boot_value = snapshot.debian.boot_time_iso.empty() ? unknown_label : snapshot.debian.boot_time_iso;

    std::ostringstream load_stream;
    load_stream << std::fixed << std::setprecision(2) << snapshot.debian.load_average[0] << " / "
                << snapshot.debian.load_average[1] << " / " << snapshot.debian.load_average[2];
    const std::string load_average_text = load_stream.str();

    const std::string initial_json = system_status_to_json(snapshot);

    append("<!DOCTYPE html>");
    append(std::string("<html lang=\"") + lang_code + "\">");
    append("<head>");
    append("  <meta charset=\"UTF-8\" />");
    append("  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />");
    append(std::string("  <title>") + html_escape(beaversystem_label) + "</title>");
    append("  <link rel=\"stylesheet\" href=\"" + resolve_asset_path(asset_prefix, "css/styles.css") + "\" />");
    append("</head>");
    append("<body>");
    append("  <div id=\"root\">");
    append("    <div class=\"beaversystem-root\">");
    append("      <header class=\"system-header\">");
    append("        <a class=\"system-header__back\" href=\"" + menu_href + "\">" +
           html_escape(back_to_menu) + "</a>");
    append("        <h1 class=\"system-header__title\">" + html_escape(beaversystem_label) + "</h1>");
    append("        <nav class=\"lang-toggle\" role=\"group\" aria-label=\"" + html_escape(language_label) + "\">");
    append(language_toggle_button("FR", beaversystem_french_href, switch_to_french, language == Language::French));
    append(language_toggle_button("EN", beaversystem_english_href, switch_to_english, language == Language::English));
    append("        </nav>");
    append("        <div class=\"system-header__accent\" aria-hidden=\"true\"></div>");
    append("      </header>");
    append("      <main class=\"system-dashboard\"");
    append("            data-label-unavailable=\"" + html_escape(unavailable_label) + "\"");
    append("            data-label-connected=\"" + html_escape(connected_label) + "\"");
    append("            data-label-not-connected=\"" + html_escape(not_connected_label) + "\"");
    append("            data-label-no-ports=\"" + html_escape(no_ports_label) + "\"");
    append("            data-label-no-telemetry=\"" + html_escape(no_telemetry_label) + "\"");
    append("            data-label-updated=\"" + html_escape(updated_label) + "\"");
    append("            data-label-interface=\"" + html_escape(interface_label) + "\"");
    append("            data-label-unknown=\"" + html_escape(unknown_label) + "\"");
    append("            data-battery-label-charging=\"" + html_escape(charging_label) + "\"");
    append("            data-battery-label-discharging=\"" + html_escape(discharging_label) + "\"");
    append("            data-battery-label-full=\"" + html_escape(full_label) + "\"");
    append("            data-battery-label-not-charging=\"" + html_escape(not_charging_label) + "\"");
    append("            data-battery-label-unavailable=\"" + html_escape(unavailable_label) + "\"");
    append("            data-battery-label-unknown=\"" + html_escape(unknown_label) + "\">");
    append("        <section class=\"system-section\">");
    append("          <div class=\"system-section__header\">");
    append("            <h2 class=\"system-section__title\">" + html_escape(system_status_title) + "</h2>");
    append("            <p class=\"system-section__meta\">" + html_escape(updated_label) + ": <span data-role=\"updated-value\">" + html_escape(last_updated_value) + "</span></p>");
    append("          </div>");
    append("          <div class=\"system-section__grid\">");
    append("            <article class=\"system-card\">");
    append("              <h3 class=\"system-card__title\">" + html_escape(home_wifi_label) + "</h3>");
    append("              <dl class=\"system-card__metrics\">");
    append("                <div class=\"system-card__metric\">");
    append("                  <dt class=\"system-card__label\">" + html_escape(status_label) + "</dt>");
    append("                  <dd class=\"system-card__value\">");
    append("                    <span class=\"status-indicator status-indicator--idle\" data-role=\"wifi-status\">" + html_escape(unavailable_label) + "</span>");
    append("                  </dd>");
    append("                </div>");
    append("                <div class=\"system-card__metric\" data-role=\"wifi-interface-row\" hidden>");
    append("                  <dt class=\"system-card__label\">" + html_escape(interface_label) + "</dt>");
    append("                  <dd class=\"system-card__value\" data-role=\"wifi-interface\">" + html_escape(unavailable_label) + "</dd>");
    append("                </div>");
    append("              </dl>");
    append("            </article>");
    append("            <article class=\"system-card\">");
    append("              <h3 class=\"system-card__title\">" + html_escape(websocket_server_label) + "</h3>");
    append("              <dl class=\"system-card__metrics\">");
    append("                <div class=\"system-card__metric\">");
    append("                  <dt class=\"system-card__label\">" + html_escape(status_label) + "</dt>");
    append("                  <dd class=\"system-card__value\">");
    append("                    <span class=\"status-indicator status-indicator--idle\" data-role=\"ws-status\">" + html_escape(unavailable_label) + "</span>");
    append("                  </dd>");
    append("                </div>");
    append("                <div class=\"system-card__metric\">");
    append("                  <dt class=\"system-card__label\">" + html_escape(last_message_label) + "</dt>");
    append("                  <dd class=\"system-card__value system-card__value--wrap\" data-role=\"ws-last-message\">" + html_escape(no_telemetry_label) + "</dd>");
    append("                </div>");
    append("              </dl>");
    append("            </article>");
    append("            <article class=\"system-card\">");
    append("              <h3 class=\"system-card__title\">" + html_escape(system_battery_label) + "</h3>");
    append("              <dl class=\"system-card__metrics\">");
    append("                <div class=\"system-card__metric\">");
    append("                  <dt class=\"system-card__label\">" + html_escape(charge_label) + "</dt>");
    append("                  <dd class=\"system-card__value\" data-role=\"battery-status\">" + html_escape(unavailable_label) + "</dd>");
    append("                </div>");
    append("              </dl>");
    append("            </article>");
    append("          </div>");
    append("        </section>");
    append("        <section class=\"system-section\">");
    append("          <div class=\"system-section__header\">");
    append("            <h2 class=\"system-section__title\">" + html_escape(resource_usage_title) + "</h2>");
    append("          </div>");
    append("          <div class=\"system-section__grid\">");
    append("            <article class=\"system-card system-card--wide\">");
    append("              <h3 class=\"system-card__title\">" + html_escape(debian_uptime_label) + "</h3>");
    append("              <dl class=\"system-card__metrics\">");
    append("                <div class=\"system-card__metric\">");
    append("                  <dt class=\"system-card__label\">" + html_escape(uptime_label) + "</dt>");
    append("                  <dd class=\"system-card__value\" data-role=\"debian-uptime\">" + html_escape(debian_uptime_value) + "</dd>");
    append("                </div>");
    append("                <div class=\"system-card__metric\">");
    append("                  <dt class=\"system-card__label\">" + html_escape(boot_time_label) + "</dt>");
    append("                  <dd class=\"system-card__value\" data-role=\"debian-boot\">" + html_escape(debian_boot_value) + "</dd>");
    append("                </div>");
    append("                <div class=\"system-card__metric\">");
    append("                  <dt class=\"system-card__label\">" + html_escape(load_label) + "</dt>");
    append("                  <dd class=\"system-card__value\" data-role=\"debian-load\">" + load_average_text + "</dd>");
    append("                </div>");
    append("              </dl>");
    append("            </article>");
    append("            <article class=\"system-card\">");
    append("              <h3 class=\"system-card__title\">" + html_escape(websocket_channel_label) + "</h3>");
    append("              <dl class=\"system-card__metrics\">");
    append("                <div class=\"system-card__metric\">");
    append("                  <dt class=\"system-card__label\">" + html_escape(raw_uptime_label) + "</dt>");
    append("                  <dd class=\"system-card__value\" data-role=\"ws-uptime\">" + html_escape(unknown_label) + "</dd>");
    append("                </div>");
    append("              </dl>");
    append("            </article>");
    append("            <article class=\"system-card system-card--ports\">");
    append("              <h3 class=\"system-card__title\">" + html_escape(network_ports_label) + "</h3>");
    append("              <div class=\"system-card__body\">");
    append("                <p class=\"system-card__hint\">" + html_escape(list_open_ports_label) + "</p>");
    append("                <div class=\"system-ports\" data-role=\"ports-list\">");
    if (snapshot.network.listening_ports.empty()) {
        append("                  <p class=\"system-ports__empty\">" + html_escape(no_ports_label) + "</p>");
    } else {
        for (std::size_t i = 0; i < snapshot.network.listening_ports.size(); ++i) {
            append("                  <span class=\"system-port-pill\">" + std::to_string(snapshot.network.listening_ports[i]) + "</span>");
        }
    }
    append("                </div>");
    append("              </div>");
    append("            </article>");
    append("          </div>");
    append("        </section>");
    append("      </main>");
    append("    </div>");
    append("  </div>");
    append("  <script id=\"initial-system-status\" type=\"application/json\">");
    append(initial_json);
    append("  </script>");
    append("  <script>");
    append("    (function() {");
    append("      const doc = document;");
    append("      const root = doc.querySelector('.system-dashboard');");
    append("      if (!root) {");
    append("        return;");
    append("      }");
    append("      const dataset = root.dataset || {};");
    append("      const strings = {");
    append("        unavailable: dataset.labelUnavailable || 'Unavailable',");
    append("        connected: dataset.labelConnected || 'Connected',");
    append("        notConnected: dataset.labelNotConnected || 'Not connected',");
    append("        noPorts: dataset.labelNoPorts || 'No listening ports detected.',");
    append("        noTelemetry: dataset.labelNoTelemetry || 'No telemetry received yet.',");
    append("        updated: dataset.labelUpdated || 'Updated',");
    append("        interface: dataset.labelInterface || 'Interface',");
    append("        unknown: dataset.labelUnknown || 'Unknown',");
    append("        battery: {");
    append("          charging: dataset.batteryLabelCharging || 'Charging',");
    append("          discharging: dataset.batteryLabelDischarging || 'Discharging',");
    append("          full: dataset.batteryLabelFull || 'Full',");
    append("          notCharging: dataset.batteryLabelNotCharging || 'Not charging',");
    append("          unavailable: dataset.batteryLabelUnavailable || dataset.labelUnavailable || 'Unavailable',");
    append("          unknown: dataset.batteryLabelUnknown || dataset.labelUnknown || 'Unknown'");
    append("        }");
    append("      };");
    append("      const wifiStatusEl = doc.querySelector('[data-role=\"wifi-status\"]');");
    append("      const wifiInterfaceRow = doc.querySelector('[data-role=\"wifi-interface-row\"]');");
    append("      const wifiInterfaceEl = doc.querySelector('[data-role=\"wifi-interface\"]');");
    append("      const wsStatusEl = doc.querySelector('[data-role=\"ws-status\"]');");
    append("      const wsLastMessageEl = doc.querySelector('[data-role=\"ws-last-message\"]');");
    append("      const batteryStatusEl = doc.querySelector('[data-role=\"battery-status\"]');");
    append("      const debianUptimeEl = doc.querySelector('[data-role=\"debian-uptime\"]');");
    append("      const debianBootEl = doc.querySelector('[data-role=\"debian-boot\"]');");
    append("      const debianLoadEl = doc.querySelector('[data-role=\"debian-load\"]');");
    append("      const wsUptimeEl = doc.querySelector('[data-role=\"ws-uptime\"]');");
    append("      const portsContainer = doc.querySelector('[data-role=\"ports-list\"]');");
    append("      const updatedValueEl = doc.querySelector('[data-role=\"updated-value\"]');");
    append("      const statusClasses = ['status-indicator--ok', 'status-indicator--warn', 'status-indicator--idle'];");
    append("      const setStatus = (el, text, tone) => {");
    append("        if (!el) {");
    append("          return;");
    append("        }");
    append("        el.textContent = text;");
    append("        statusClasses.forEach((cls) => el.classList.remove(cls));");
    append("        const toneClass = tone === 'ok' ? 'status-indicator--ok' : tone === 'warn' ? 'status-indicator--warn' : 'status-indicator--idle';");
    append("        el.classList.add(toneClass);");
    append("      };");
    append("      const setText = (el, text) => {");
    append("        if (el) {");
    append("          el.textContent = text;");
    append("        }");
    append("      };");
    append("      const formatDuration = (seconds) => {");
    append("        if (!Number.isFinite(seconds) || seconds < 0) {");
    append("          return strings.unknown;");
    append("        }");
    append("        const total = Math.floor(seconds);");
    append("        const days = Math.floor(total / 86400);");
    append("        const hours = Math.floor((total % 86400) / 3600);");
    append("        const minutes = Math.floor((total % 3600) / 60);");
    append("        const secs = total % 60;");
    append("        const pad = (value) => value.toString().padStart(2, '0');");
    append("        const parts = [];");
    append("        if (days > 0) {");
    append("          parts.push(`${days}d`);");
    append("        }");
    append("        parts.push(`${pad(hours)}h`);");
    append("        parts.push(`${pad(minutes)}m`);");
    append("        parts.push(`${pad(secs)}s`);");
    append("        return parts.join(' ');");
    append("      };");
    append("      const renderPorts = (ports) => {");
    append("        if (!portsContainer) {");
    append("          return;");
    append("        }");
    append("        portsContainer.textContent = '';");
    append("        if (!Array.isArray(ports) || ports.length === 0) {");
    append("          const message = doc.createElement('p');");
    append("          message.className = 'system-ports__empty';");
    append("          message.textContent = strings.noPorts;");
    append("          portsContainer.appendChild(message);");
    append("          return;");
    append("        }");
    append("        const unique = Array.from(new Set(ports)).sort((a, b) => a - b);");
    append("        unique.forEach((port) => {");
    append("          const pill = doc.createElement('span');");
    append("          pill.className = 'system-port-pill';");
    append("          pill.textContent = port;");
    append("          portsContainer.appendChild(pill);");
    append("        });");
    append("      };");
    append("      const renderBattery = (battery) => {");
    append("        if (!batteryStatusEl) {");
    append("          return;");
    append("        }");
    append("        if (!battery || (!battery.present && !battery.state)) {");
    append("          batteryStatusEl.textContent = strings.battery.unavailable;");
    append("          return;");
    append("        }");
    append("        const stateKey = (battery.state || '').toString().toLowerCase();");
    append("        const lookup = {");
    append("          charging: strings.battery.charging,");
    append("          discharging: strings.battery.discharging,");
    append("          full: strings.battery.full,");
    append("          'not charging': strings.battery.notCharging,");
    append("          'not-charging': strings.battery.notCharging,");
    append("          unknown: strings.battery.unknown,");
    append("          unavailable: strings.battery.unavailable");
    append("        };");
    append("        const mapped = lookup[stateKey] || battery.state || strings.battery.unknown;");
    append("        let percentageText = null;");
    append("        if (typeof battery.percentage === 'number' && Number.isFinite(battery.percentage)) {");
    append("          const safePercent = Math.max(0, Math.min(100, Math.round(battery.percentage)));");
    append("          percentageText = `${safePercent}%`;");
    append("        }");
    append("        if (battery.present && percentageText) {");
    append("          batteryStatusEl.textContent = `${percentageText} – ${mapped}`;");
    append("        } else if (battery.present) {");
    append("          batteryStatusEl.textContent = mapped;");
    append("        } else {");
    append("          batteryStatusEl.textContent = mapped || strings.battery.unavailable;");
    append("        }");
    append("      };");
    append("      const renderData = (data) => {");
    append("        if (!data || typeof data !== 'object') {");
    append("          return;");
    append("        }");
    append("        const wifi = data.wifi || {};");
    append("        if (wifiStatusEl) {");
    append("          if (wifi.available) {");
    append("            if (wifi.connected) {");
    append("              setStatus(wifiStatusEl, strings.connected, 'ok');");
    append("            } else {");
    append("              setStatus(wifiStatusEl, strings.notConnected, 'warn');");
    append("            }");
    append("          } else {");
    append("            setStatus(wifiStatusEl, strings.unavailable, 'idle');");
    append("          }");
    append("        }");
    append("        if (wifiInterfaceRow && wifiInterfaceEl) {");
    append("          if (wifi.available && wifi.interface) {");
    append("            wifiInterfaceRow.hidden = false;");
    append("            setText(wifiInterfaceEl, wifi.interface);");
    append("          } else {");
    append("            wifiInterfaceRow.hidden = true;");
    append("            setText(wifiInterfaceEl, strings.unavailable);");
    append("          }");
    append("        }");
    append("        const websocket = data.websocket || {};");
    append("        if (wsStatusEl) {");
    append("          if (websocket.listening) {");
    append("            setStatus(wsStatusEl, strings.connected, 'ok');");
    append("          } else if (websocket.address) {");
    append("            setStatus(wsStatusEl, strings.notConnected, 'warn');");
    append("          } else {");
    append("            setStatus(wsStatusEl, strings.unavailable, 'idle');");
    append("          }");
    append("        }");
    append("        const lastMessage = (websocket.lastMessage || '').toString().trim();");
    append("        let fallbackMessage = strings.noTelemetry;");
    append("        if (websocket.listening) {");
    append("          fallbackMessage = strings.connected;");
    append("        } else if (websocket.address) {");
    append("          fallbackMessage = strings.notConnected;");
    append("        }");
    append("        setText(wsLastMessageEl, lastMessage ? lastMessage : fallbackMessage);");
    append("        renderBattery(data.battery);");
    append("        if (debianUptimeEl && data.debian) {");
    append("          setText(debianUptimeEl, data.debian.uptimeHuman || strings.unknown);");
    append("          setText(debianBootEl, data.debian.bootTime || strings.unknown);");
    append("          if (Array.isArray(data.debian.loadAverage) && data.debian.loadAverage.length >= 3) {");
    append("            const formatted = data.debian.loadAverage.slice(0, 3).map((value) => {");
    append("              return Number.isFinite(value) ? Number(value).toFixed(2) : '0.00';");
    append("            }).join(' / ');");
    append("            setText(debianLoadEl, formatted);");
    append("          } else {");
    append("            setText(debianLoadEl, strings.unknown);");
    append("          }");
    append("        }");
    append("        if (wsUptimeEl) {");
    append("          const uptimeSeconds = websocket.uptimeSeconds;");
    append("          setText(wsUptimeEl, formatDuration(typeof uptimeSeconds === 'number' ? uptimeSeconds : -1));");
    append("        }");
    append("        renderPorts(data.network ? data.network.listeningPorts : null);");
    append("        if (updatedValueEl) {");
    append("          setText(updatedValueEl, data.generatedAt || strings.unknown);");
    append("        }");
    append("      };");
    append("      const parseInitial = () => {");
    append("        const script = doc.getElementById('initial-system-status');");
    append("        if (!script) {");
    append("          return null;");
    append("        }");
    append("        try {");
    append("          return JSON.parse(script.textContent || '{}');");
    append("        } catch (error) {");
    append("          console.warn('[BeaverSystem] Unable to parse initial system status payload.', error);");
    append("          return null;");
    append("        }");
    append("      };");
    append("      const fetchLatest = () => {");
    append("        if (typeof fetch !== 'function') {");
    append("          return;");
    append("        }");
    append("        fetch('/api/system/status', { cache: 'no-cache' })");
    append("          .then((response) => {");
    append("            if (!response.ok) {");
    append("              throw new Error(`HTTP ${response.status}`);");
    append("            }");
    append("            return response.json();");
    append("          })");
    append("          .then((payload) => {");
    append("            renderData(payload);");
    append("          })");
    append("          .catch((error) => {");
    append("            console.warn('[BeaverSystem] Failed to refresh system status.', error);");
    append("          });");
    append("      };");
    append("      const initial = parseInitial();");
    append("      if (initial) {");
    append("        renderData(initial);");
    append("      }");
    append("      if (window.location && (window.location.protocol === 'http:' || window.location.protocol === 'https:')) {");
    append("        fetchLatest();");
    append("        window.setInterval(fetchLatest, 15000);");
    append("      }");
    append("    })();");
    append("  </script>");
    append("</body>");
    append("</html>");

    return html.str();
}

std::string generate_beavertask_planner_html(const TranslationCatalog& translations,
                                             Language language, const std::string& asset_prefix,
                                             BeaverTaskMenuLinkMode menu_link_mode) {
    const char* lang_code = html_lang_code(language);
    const std::string beavertask_label = translations.translate("BeaverTask", language);
    const std::string back_to_menu = translations.translate("Back to menu", language);
    const std::string language_label = translations.translate("Language selection", language);
    const std::string switch_to_french = translations.translate("Switch to French", language);
    const std::string switch_to_english = translations.translate("Switch to English", language);
    const std::string task_list_header = translations.translate("Task list header", language);
    const std::string task_list_empty = translations.translate("Task list empty", language);
    const std::string details_empty = translations.translate("Details empty", language);
    const std::string new_event_title = translations.translate("New event title", language);
    const std::string new_event_description =
        translations.translate("New event description", language);
    const std::string add_event_template = translations.translate("Add event for DATE", language);
    const std::string title_label = translations.translate("Title label", language);
    const std::string title_placeholder = translations.translate("Title placeholder", language);
    const std::string all_day_label = translations.translate("All day label", language);
    const std::string start_time_label = translations.translate("Start time label", language);
    const std::string end_time_label = translations.translate("End time label", language);
    const std::string location_label = translations.translate("Location label", language);
    const std::string location_placeholder =
        translations.translate("Location placeholder", language);
    const std::string color_label = translations.translate("Color label", language);
    const std::string reminder_label = translations.translate("Reminder label", language);
    const std::string reminder_none = translations.translate("Reminder none", language);
    const std::string reminder_5 = translations.translate("Reminder 5 minutes", language);
    const std::string reminder_10 = translations.translate("Reminder 10 minutes", language);
    const std::string reminder_15 = translations.translate("Reminder 15 minutes", language);
    const std::string reminder_30 = translations.translate("Reminder 30 minutes", language);
    const std::string reminder_60 = translations.translate("Reminder 1 hour", language);
    const std::string reminder_day = translations.translate("Reminder 1 day", language);
    const std::string category_label = translations.translate("Category label", language);
    const std::string category_placeholder =
        translations.translate("Category placeholder", language);
    const std::string notes_label = translations.translate("Notes label", language);
    const std::string notes_placeholder = translations.translate("Notes placeholder", language);
    const std::string cancel_button = translations.translate("Cancel button", language);
    const std::string save_button = translations.translate("Save button", language);
    const std::string date_jump_title = translations.translate("Date jump title", language);
    const std::string date_jump_description =
        translations.translate("Date jump description", language);
    const std::string date_label = translations.translate("Date label", language);
    const std::string date_jump_go = translations.translate("Date jump go", language);
    const std::string date_jump_cancel = translations.translate("Date jump cancel", language);
    const std::string add_event_aria = translations.translate("Add event aria label", language);
    const std::string search_date_aria = translations.translate("Search date aria label", language);
    const std::string calendar_view_aria =
        translations.translate("Calendar view aria label", language);
    const std::string task_list_view_aria =
        translations.translate("Task list view aria label", language);
    const std::string previous_month_label = translations.translate("Previous month", language);
    const std::string next_month_label = translations.translate("Next month", language);
    const std::string confirm_delete = translations.translate("Confirm delete task", language);
    const std::string meta_all_day = translations.translate("Task meta all day", language);
    const std::string reminder_minutes_fmt =
        translations.translate("Reminder minutes format", language);
    const std::string reminder_hours_fmt =
        translations.translate("Reminder hours format", language);
    const std::string reminder_days_fmt = translations.translate("Reminder days format", language);
    const std::string no_due_date_label = translations.translate("No due date", language);
    const std::string group_overdue = translations.translate("Task group overdue", language);
    const std::string group_upcoming = translations.translate("Task group upcoming", language);
    const std::string group_no_due = translations.translate("Task group no due date", language);
    const std::string group_completed = translations.translate("Task group completed", language);

    const std::array<std::string, 12> month_names_short_en = {
        "JAN.", "FEB.", "MAR.", "APR.", "MAY", "JUN.", "JUL.", "AUG.", "SEP.", "OCT.", "NOV.", "DEC."};
    const std::array<std::string, 12> month_names_short_fr = {
        "JANV.", "FÉV.", "MARS", "AVR.", "MAI", "JUIN", "JUIL.", "AOÛT", "SEPT.", "OCT.", "NOV.", "DÉC."};
    const std::array<std::string, 12> month_names_detail_en = {
        "Jan.", "Feb.", "Mar.", "Apr.", "May", "Jun.", "Jul.", "Aug.", "Sep.", "Oct.", "Nov.", "Dec."};
    const std::array<std::string, 12> month_names_detail_fr = {
        "janv.", "févr.", "mars", "avr.", "mai", "juin", "juil.", "août", "sept.", "oct.", "nov.", "déc."};
    const std::array<std::string, 12> month_names_full_en = {
        "January", "February", "March", "April", "May", "June", "July", "August", "September",
        "October", "November", "December"};
    const std::array<std::string, 12> month_names_full_fr = {
        "janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre",
        "octobre", "novembre", "décembre"};
    const std::array<std::string, 7> weekday_labels_en = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    const std::array<std::string, 7> weekday_labels_fr = {"DIM", "LUN", "MAR", "MER", "JEU", "VEN", "SAM"};

    const auto& month_names_short =
        (language == Language::French) ? month_names_short_fr : month_names_short_en;
    const auto& month_names_detail =
        (language == Language::French) ? month_names_detail_fr : month_names_detail_en;
    const auto& month_names_full =
        (language == Language::French) ? month_names_full_fr : month_names_full_en;
    const auto& weekday_labels =
        (language == Language::French) ? weekday_labels_fr : weekday_labels_en;
    const std::string locale_code = (language == Language::French) ? "fr-CA" : "en-CA";

    std::vector<std::string> json_entries;
    json_entries.emplace_back(std::string("\"locale\": \"") + json_escape(locale_code) + "\"");
    json_entries.emplace_back(std::string("\"monthNamesShort\": ") +
                              json_array(std::vector<std::string>(month_names_short.begin(),
                                                                  month_names_short.end())));
    json_entries.emplace_back(std::string("\"monthNamesDetail\": ") +
                              json_array(std::vector<std::string>(month_names_detail.begin(),
                                                                  month_names_detail.end())));
    json_entries.emplace_back(std::string("\"monthNamesFull\": ") +
                              json_array(std::vector<std::string>(month_names_full.begin(),
                                                                  month_names_full.end())));
    json_entries.emplace_back(std::string("\"weekdayLabels\": ") +
                              json_array(std::vector<std::string>(weekday_labels.begin(),
                                                                  weekday_labels.end())));
    json_entries.emplace_back(std::string("\"emptyDetails\": \"") + json_escape(details_empty) + "\"");
    json_entries.emplace_back(std::string("\"taskListEmpty\": \"") + json_escape(task_list_empty) + "\"");
    json_entries.emplace_back(std::string("\"modalDateTemplate\": \"") +
                              json_escape(add_event_template) + "\"");
    json_entries.emplace_back(std::string("\"confirmDelete\": \"") + json_escape(confirm_delete) + "\"");
    json_entries.emplace_back(std::string("\"metaAllDay\": \"") + json_escape(meta_all_day) + "\"");
    json_entries.emplace_back(std::string("\"reminderMinutes\": \"") +
                              json_escape(reminder_minutes_fmt) + "\"");
    json_entries.emplace_back(std::string("\"reminderHours\": \"") +
                              json_escape(reminder_hours_fmt) + "\"");
    json_entries.emplace_back(std::string("\"reminderDays\": \"") +
                              json_escape(reminder_days_fmt) + "\"");
    json_entries.emplace_back(std::string("\"noDueDate\": \"") + json_escape(no_due_date_label) + "\"");
    json_entries.emplace_back(std::string("\"groupOverdue\": \"") + json_escape(group_overdue) + "\"");
    json_entries.emplace_back(std::string("\"groupUpcoming\": \"") + json_escape(group_upcoming) + "\"");
    json_entries.emplace_back(std::string("\"groupNoDueDate\": \"") + json_escape(group_no_due) + "\"");
    json_entries.emplace_back(std::string("\"groupCompleted\": \"") + json_escape(group_completed) + "\"");
    json_entries.emplace_back(std::string("\"taskListHeader\": \"") + json_escape(task_list_header) + "\"");
    json_entries.emplace_back(std::string("\"searchDateAria\": \"") + json_escape(search_date_aria) + "\"");
    json_entries.emplace_back(std::string("\"addEventAria\": \"") + json_escape(add_event_aria) + "\"");
    json_entries.emplace_back(std::string("\"calendarViewAria\": \"") +
                              json_escape(calendar_view_aria) + "\"");
    json_entries.emplace_back(std::string("\"taskListViewAria\": \"") +
                              json_escape(task_list_view_aria) + "\"");
    json_entries.emplace_back(std::string("\"previousMonth\": \"") +
                              json_escape(previous_month_label) + "\"");
    json_entries.emplace_back(std::string("\"nextMonth\": \"") + json_escape(next_month_label) + "\"");

    std::ostringstream json_payload;
    json_payload << "{\n";
    for (std::size_t i = 0; i < json_entries.size(); ++i) {
        json_payload << "  " << json_entries[i];
        if (i + 1 < json_entries.size()) {
            json_payload << ",";
        }
        json_payload << "\n";
    }
    json_payload << "}";

    const std::string menu_href = build_menu_href(language, menu_link_mode);
    const bool use_absolute_links = (menu_link_mode == BeaverTaskMenuLinkMode::kAbsoluteRoot);
    const std::string beavertask_base = use_absolute_links ? "/apps/beavertask" : "apps/beavertask";
    const std::string beavertask_french_href = beavertask_base + "?lang=fr";
    const std::string beavertask_english_href = beavertask_base + "?lang=en";

    std::ostringstream html;
    auto append = [&](const std::string& line) { html << line << '\n'; };

    append("<!DOCTYPE html>");
    append(std::string("<html lang=\"") + lang_code + "\">");
    append("<head>");
    append("  <meta charset=\"UTF-8\" />");
    append("  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />");
    append("  <title>" + html_escape(beavertask_label) + " - BeaverKiosk</title>");
    append("  <link rel=\"stylesheet\" href=\"" +
           resolve_asset_path(asset_prefix, "css/styles.css") + "\" />");
    append("</head>");
    append("<body>");
    append("  <div id=\"root\">");
    append("    <div class=\"beavertask-page\" data-locale=\"" + html_escape(locale_code) + "\">");
    append("      <aside class=\"sidebar\">");
    append("        <a class=\"sidebar-back\" href=\"" + menu_href + "\" aria-label=\"" +
           html_escape(back_to_menu) + "\">");
    append("          <svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">");
    append("            <path d=\"M3 10a2 2 0 0 1 .709-1.528l7-6a2 2 0 0 1 2.582 0l7 6A2 2 0 0 1 21 10v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z\"></path>");
    append("            <path d=\"M15 21v-8a1 1 0 0 0-1-1h-4a1 1 0 0 0-1 1v8\"></path>");
    append("          </svg>");
    append("        </a>");
    append("        <button type=\"button\" class=\"sidebar-icon active\" id=\"calendar-icon\" aria-label=\"" +
           html_escape(calendar_view_aria) + "\">");
    append("          <svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">");
    append("            <rect x=\"3\" y=\"4\" width=\"18\" height=\"18\" rx=\"2\" ry=\"2\"></rect>");
    append("            <line x1=\"16\" y1=\"2\" x2=\"16\" y2=\"6\"></line>");
    append("            <line x1=\"8\" y1=\"2\" x2=\"8\" y2=\"6\"></line>");
    append("            <line x1=\"3\" y1=\"10\" x2=\"21\" y2=\"10\"></line>");
    append("          </svg>");
    append("        </button>");
    append("        <button type=\"button\" class=\"sidebar-icon\" id=\"tasklist-icon\" aria-label=\"" +
           html_escape(task_list_view_aria) + "\">");
    append("          <svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">");
    append("            <line x1=\"8\" y1=\"6\" x2=\"21\" y2=\"6\"></line>");
    append("            <line x1=\"8\" y1=\"12\" x2=\"21\" y2=\"12\"></line>");
    append("            <line x1=\"8\" y1=\"18\" x2=\"21\" y2=\"18\"></line>");
    append("            <line x1=\"3\" y1=\"6\" x2=\"3.01\" y2=\"6\"></line>");
    append("            <line x1=\"3\" y1=\"12\" x2=\"3.01\" y2=\"12\"></line>");
    append("            <line x1=\"3\" y1=\"18\" x2=\"3.01\" y2=\"18\"></line>");
    append("          </svg>");
    append("        </button>");
    append("      </aside>");
    append("      <div class=\"main-content\">");
    append("        <header class=\"task-header\">");
    append("          <h1 class=\"task-title\">" + html_escape(beavertask_label) + "</h1>");
    append("          <nav class=\"lang-toggle\" role=\"group\" aria-label=\"" +
           html_escape(language_label) + "\">");
    append(language_toggle_button("FR", beavertask_french_href, switch_to_french,
                                   language == Language::French));
    append(language_toggle_button("EN", beavertask_english_href, switch_to_english,
                                   language == Language::English));
    append("          </nav>");
    append("        </header>");
    append("        <div class=\"task-layout\">");
    append("          <section class=\"calendar-section\" aria-labelledby=\"month-display\">");
    append("            <div class=\"calendar-header\">");
    append("              <button class=\"icon-btn\" id=\"prev-month\" type=\"button\" aria-label=\"" +
           html_escape(previous_month_label) + "\" title=\"" + html_escape(previous_month_label) + "\">");
    append("                <svg width=\"20\" height=\"20\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">");
    append("                  <polyline points=\"15 18 9 12 15 6\"></polyline>");
    append("                </svg>");
    append("              </button>");
    append("              <div class=\"month-display\" id=\"month-display\">" +
           html_escape(month_names_short[0]) + "</div>");
    append("              <button class=\"icon-btn\" id=\"next-month\" type=\"button\" aria-label=\"" +
           html_escape(next_month_label) + "\" title=\"" + html_escape(next_month_label) + "\">");
    append("                <svg width=\"20\" height=\"20\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">");
    append("                  <polyline points=\"9 18 15 12 9 6\"></polyline>");
    append("                </svg>");
    append("              </button>");
    append("              <div class=\"header-controls\">");
    append("                <button class=\"icon-btn\" id=\"search-btn\" type=\"button\" aria-label=\"" +
           html_escape(search_date_aria) + "\" title=\"" + html_escape(search_date_aria) + "\">");
    append("                  <svg width=\"20\" height=\"20\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">");
    append("                    <circle cx=\"11\" cy=\"11\" r=\"8\"></circle>");
    append("                    <path d=\"m21 21-4.35-4.35\"></path>");
    append("                  </svg>");
    append("                </button>");
    append("                <div class=\"current-day-badge\" id=\"current-day-badge\">--</div>");
    append("              </div>");
    append("            </div>");
    append("            <div class=\"weekdays\">");
    for (std::size_t i = 0; i < weekday_labels.size(); ++i) {
        const bool is_dim = (i == 0);
        html << "              <div class=\"weekday" << (is_dim ? " dim" : "") << "\">"
             << html_escape(weekday_labels[i]) << "</div>\n";
    }
    append("            </div>");
    append("            <div class=\"calendar-grid\" id=\"calendar-grid\"></div>");
    append("          </section>");
    append("          <section class=\"details-panel\">");
    append("            <div class=\"details-header\">");
    append("              <div class=\"details-date\" id=\"details-header\">--</div>");
    append("              <button class=\"details-add-btn\" id=\"details-add-btn\" type=\"button\" aria-label=\"" +
           html_escape(add_event_aria) + "\">+</button>");
    append("            </div>");
    append("            <div class=\"details-content\" id=\"details-content\">");
    append("              <div class=\"empty-state\">");
    append("                <div class=\"empty-icon\">☺</div>");
    append("                <div class=\"empty-text\">" + html_escape(details_empty) + "</div>");
    append("              </div>");
    append("            </div>");
    append("          </section>");
    append("          <section class=\"tasklist-section\" id=\"tasklist-section\">");
    append("            <div class=\"tasklist-header\" id=\"tasklist-header\">" +
           html_escape(task_list_header) + "</div>");
    append("            <div class=\"tasklist-content\" id=\"tasklist-content\">");
    append("              <div class=\"tasklist-empty\">");
    append("                <div class=\"empty-icon\">📋</div>");
    append("                <div class=\"empty-text\">" + html_escape(task_list_empty) + "</div>");
    append("              </div>");
    append("            </div>");
    append("          </section>");
    append("        </div>");
    append("      </div>");
    append("      <div class=\"modal-overlay\" id=\"task-modal\" hidden>");
    append("        <div class=\"modal-card\" role=\"dialog\" aria-modal=\"true\" aria-labelledby=\"task-modal-title\">");
    append("          <h2 class=\"modal-title\" id=\"task-modal-title\">" +
           html_escape(new_event_title) + "</h2>");
    append("          <p class=\"modal-description\" id=\"task-modal-description\">" +
           html_escape(new_event_description) + "</p>");
    append("          <form class=\"modal-form\" id=\"task-form\">");
    append("            <div class=\"modal-field\">");
    append("              <label class=\"modal-label\" for=\"task-title\">" +
           html_escape(title_label) + "</label>");
    append("              <input class=\"modal-input\" type=\"text\" id=\"task-title\" name=\"title\" placeholder=\"" +
           html_escape(title_placeholder) + "\" required autocomplete=\"off\" />");
    append("            </div>");
    append("            <div class=\"modal-field modal-field-toggle\">");
    append("              <label class=\"modal-label\" for=\"task-allday\">" +
           html_escape(all_day_label) + "</label>");
    append("              <label class=\"toggle-switch\">");
    append("                <input type=\"checkbox\" id=\"task-allday\" name=\"allDay\">");
    append("                <span class=\"toggle-slider\"></span>");
    append("              </label>");
    append("            </div>");
    append("            <div class=\"modal-field-group\" id=\"time-fields\">");
    append("              <div class=\"modal-field\">");
    append("                <label class=\"modal-label\" for=\"task-start-time\">" +
           html_escape(start_time_label) + "</label>");
    append("                <input class=\"modal-input\" type=\"time\" id=\"task-start-time\" name=\"startTime\" value=\"09:00\" />");
    append("              </div>");
    append("              <div class=\"modal-field\">");
    append("                <label class=\"modal-label\" for=\"task-end-time\">" +
           html_escape(end_time_label) + "</label>");
    append("                <input class=\"modal-input\" type=\"time\" id=\"task-end-time\" name=\"endTime\" value=\"10:00\" />");
    append("              </div>");
    append("            </div>");
    append("            <div class=\"modal-field\">");
    append("              <label class=\"modal-label\" for=\"task-location\">");
    append("                <svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">");
    append("                  <path d=\"M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z\"></path>");
    append("                  <circle cx=\"12\" cy=\"10\" r=\"3\"></circle>");
    append("                </svg>");
    append("                " + html_escape(location_label));
    append("              </label>");
    append("              <input class=\"modal-input\" type=\"text\" id=\"task-location\" name=\"location\" placeholder=\"" +
           html_escape(location_placeholder) + "\" autocomplete=\"off\" />");
    append("            </div>");
    append("            <div class=\"modal-field\">");
    append("              <label class=\"modal-label\" for=\"task-color\">");
    append("                <svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">");
    append("                  <circle cx=\"12\" cy=\"12\" r=\"10\"></circle>");
    append("                </svg>");
    append("                " + html_escape(color_label));
    append("              </label>");
    append("              <div class=\"color-picker\">");
    append("                <label class=\"color-option\">");
    append("                  <input type=\"radio\" name=\"color\" value=\"#f89422\" checked>");
    append("                  <span class=\"color-swatch\" style=\"background: #f89422;\"></span>");
    append("                </label>");
    append("                <label class=\"color-option\">");
    append("                  <input type=\"radio\" name=\"color\" value=\"#3b82f6\">");
    append("                  <span class=\"color-swatch\" style=\"background: #3b82f6;\"></span>");
    append("                </label>");
    append("                <label class=\"color-option\">");
    append("                  <input type=\"radio\" name=\"color\" value=\"#ef4444\">");
    append("                  <span class=\"color-swatch\" style=\"background: #ef4444;\"></span>");
    append("                </label>");
    append("                <label class=\"color-option\">");
    append("                  <input type=\"radio\" name=\"color\" value=\"#10b981\">");
    append("                  <span class=\"color-swatch\" style=\"background: #10b981;\"></span>");
    append("                </label>");
    append("                <label class=\"color-option\">");
    append("                  <input type=\"radio\" name=\"color\" value=\"#8b5cf6\">");
    append("                  <span class=\"color-swatch\" style=\"background: #8b5cf6;\"></span>");
    append("                </label>");
    append("                <label class=\"color-option\">");
    append("                  <input type=\"radio\" name=\"color\" value=\"#ec4899\">");
    append("                  <span class=\"color-swatch\" style=\"background: #ec4899;\"></span>");
    append("                </label>");
    append("              </div>");
    append("            </div>");
    append("            <div class=\"modal-field\">");
    append("              <label class=\"modal-label\" for=\"task-reminder\">");
    append("                <svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">");
    append("                  <path d=\"M18 8A6 6 0 0 0 6 8c0 7-3 9-3 9h18s-3-2-3-9\"></path>");
    append("                  <path d=\"M13.73 21a2 2 0 0 1-3.46 0\"></path>");
    append("                </svg>");
    append("                " + html_escape(reminder_label));
    append("              </label>");
    append("              <select class=\"modal-input\" id=\"task-reminder\" name=\"reminder\">");
    append("                <option value=\"0\">" + html_escape(reminder_none) + "</option>");
    append("                <option value=\"5\">" + html_escape(reminder_5) + "</option>");
    append("                <option value=\"10\" selected>" + html_escape(reminder_10) + "</option>");
    append("                <option value=\"15\">" + html_escape(reminder_15) + "</option>");
    append("                <option value=\"30\">" + html_escape(reminder_30) + "</option>");
    append("                <option value=\"60\">" + html_escape(reminder_60) + "</option>");
    append("                <option value=\"1440\">" + html_escape(reminder_day) + "</option>");
    append("              </select>");
    append("            </div>");
    append("            <div class=\"modal-field\">");
    append("              <label class=\"modal-label\" for=\"task-tag\">" +
           html_escape(category_label) + "</label>");
    append("              <input class=\"modal-input\" type=\"text\" id=\"task-tag\" name=\"tag\" placeholder=\"" +
           html_escape(category_placeholder) + "\" autocomplete=\"off\" />");
    append("            </div>");
    append("            <div class=\"modal-field\">");
    append("              <label class=\"modal-label\" for=\"task-description\">" +
           html_escape(notes_label) + "</label>");
    append("              <textarea class=\"modal-textarea\" id=\"task-description\" name=\"description\" placeholder=\"" +
           html_escape(notes_placeholder) + "\"></textarea>");
    append("            </div>");
    append("            <div class=\"modal-actions\">");
    append("              <button type=\"button\" class=\"modal-btn secondary\" id=\"task-cancel\">" +
           html_escape(cancel_button) + "</button>");
    append("              <button type=\"submit\" class=\"modal-btn primary\">" +
           html_escape(save_button) + "</button>");
    append("            </div>");
    append("          </form>");
    append("        </div>");
    append("      </div>");
    append("      <div class=\"modal-overlay\" id=\"date-jump-modal\" hidden>");
    append("        <div class=\"modal-card\" role=\"dialog\" aria-modal=\"true\" aria-labelledby=\"date-jump-title\">");
    append("          <h2 class=\"modal-title\" id=\"date-jump-title\">" +
           html_escape(date_jump_title) + "</h2>");
    append("          <p class=\"modal-description\">" + html_escape(date_jump_description) + "</p>");
    append("          <form class=\"modal-form\" id=\"date-jump-form\">");
    append("            <div class=\"modal-field\">");
    append("              <label class=\"modal-label\" for=\"jump-date\">" + html_escape(date_label) + "</label>");
    append("              <input class=\"modal-input\" type=\"date\" id=\"jump-date\" name=\"date\" required />");
    append("            </div>");
    append("            <div class=\"modal-actions\">");
    append("              <button type=\"button\" class=\"modal-btn secondary\" id=\"date-jump-cancel\">" +
           html_escape(date_jump_cancel) + "</button>");
    append("              <button type=\"submit\" class=\"modal-btn primary\">" +
           html_escape(date_jump_go) + "</button>");
    append("            </div>");
    append("          </form>");
    append("        </div>");
    append("      </div>");
    append("    </div>");
    append("  </div>");
    append("  <script id=\"beavertask-strings\" type=\"application/json\">");
    append(json_payload.str());
    append("  </script>");
    append("  <script>");
    append("    (function() {");
    append("      const root = document.querySelector('.beavertask-page');");
    append("      if (!root) {");
    append("        return;");
    append("      }");
    append("      const stringsScript = document.getElementById('beavertask-strings');");
    append("      let strings = {};");
    append("      if (stringsScript) {");
    append("        try {");
    append("          strings = JSON.parse(stringsScript.textContent || '{}') || {};");
    append("        } catch (error) {");
    append("          console.warn('[BeaverTask] Failed to parse translation payload.', error);");
    append("        }");
    append("      }");
    append("      const fallbackMonthsShort = ['JAN.', 'FEB.', 'MAR.', 'APR.', 'MAY', 'JUN.', 'JUL.', 'AUG.', 'SEP.', 'OCT.', 'NOV.', 'DEC.'];");
    append("      const fallbackMonthsDetail = ['Jan.', 'Feb.', 'Mar.', 'Apr.', 'May', 'Jun.', 'Jul.', 'Aug.', 'Sep.', 'Oct.', 'Nov.', 'Dec.'];");
    append("      const fallbackMonthsFull = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];");
    append("      const fallbackWeekdays = ['SUN', 'MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT'];");
    append("      const monthNamesShort = Array.isArray(strings.monthNamesShort) && strings.monthNamesShort.length === 12 ? strings.monthNamesShort : fallbackMonthsShort;");
    append("      const monthNamesDetail = Array.isArray(strings.monthNamesDetail) && strings.monthNamesDetail.length === 12 ? strings.monthNamesDetail : fallbackMonthsDetail;");
    append("      const monthNamesFull = Array.isArray(strings.monthNamesFull) && strings.monthNamesFull.length === 12 ? strings.monthNamesFull : fallbackMonthsFull;");
    append("      const weekdayLabels = Array.isArray(strings.weekdayLabels) && strings.weekdayLabels.length === 7 ? strings.weekdayLabels : fallbackWeekdays;");
    append("      const locale = typeof strings.locale === 'string' && strings.locale ? strings.locale : 'en-CA';");
    append("      const storageKey = 'beavertask.tasks';");
    append("      const state = {");
    append("        tasks: [],");
    append("        selectedDate: new Date(),");
    append("        currentYear: new Date().getFullYear(),");
    append("        currentMonth: new Date().getMonth()");
    append("      };");
    append("      const getDateKey = (date) => {");
    append("        const year = date.getFullYear();");
    append("        const month = String(date.getMonth() + 1).padStart(2, '0');");
    append("        const day = String(date.getDate()).padStart(2, '0');");
    append("        return `${year}-${month}-${day}`;");
    append("      };");
    append("      const parseDateValue = (value) => {");
    append("        if (!value || typeof value !== 'string') {");
    append("          return null;");
    append("        }");
    append("        const parts = value.split('-').map((part) => Number.parseInt(part, 10));");
    append("        if (parts.length !== 3 || parts.some((part) => Number.isNaN(part))) {");
    append("          return null;");
    append("        }");
    append("        return new Date(parts[0], parts[1] - 1, parts[2]);");
    append("      };");
    append("      const escapeHtml = (value) => {");
    append("        if (typeof value !== 'string') {");
    append("          return '';");
    append("        }");
    append("        return value.replace(/[&<>'\"]+/g, (char) => {");
    append("          switch (char) {");
    append("            case '&': return '&amp;';");
    append("            case '<': return '&lt;';");
    append("            case '>': return '&gt;';");
    append("            case '\'': return '&#39;';");
    append("            case '\"': return '&quot;';");
    append("            default: return char;");
    append("          }");
    append("        });");
    append("      };");
    append("      const sanitizeColor = (value) => {");
    append("        if (typeof value !== 'string') {");
    append("          return '';");
    append("        }");
    append("        return /^#[0-9a-fA-F]{3,8}$/.test(value) ? value : '';");
    append("      };");
    append("      const normalizeTasks = (items) => {");
    append("        if (!Array.isArray(items)) {");
    append("          return [];");
    append("        }");
    append("        return items.filter((item) => item && typeof item === 'object').map((item) => {");
    append("          return {");
    append("            id: typeof item.id === 'string' && item.id ? item.id : `task_${Date.now().toString(36)}${Math.random().toString(36).slice(2, 8)}`,");
    append("            title: typeof item.title === 'string' ? item.title : '',");
    append("            description: typeof item.description === 'string' ? item.description : '',");
    append("            tag: typeof item.tag === 'string' ? item.tag : '',");
    append("            dueDate: typeof item.dueDate === 'string' ? item.dueDate : '',");
    append("            startTime: typeof item.startTime === 'string' ? item.startTime : '',");
    append("            endTime: typeof item.endTime === 'string' ? item.endTime : '',");
    append("            allDay: Boolean(item.allDay),");
    append("            location: typeof item.location === 'string' ? item.location : '',");
    append("            reminderMinutes: Number.isFinite(Number(item.reminderMinutes)) ? Number(item.reminderMinutes) : 0,");
    append("            color: typeof item.color === 'string' ? item.color : '',");
    append("            completed: Boolean(item.completed)");
    append("          };");
    append("        });");
    append("      };");
    append("      const loadTasks = () => {");
    append("        try {");
    append("          const raw = window.localStorage.getItem(storageKey);");
    append("          if (!raw) {");
    append("            return [];");
    append("          }");
    append("          const parsed = JSON.parse(raw);");
    append("          return normalizeTasks(parsed);");
    append("        } catch (error) {");
    append("          console.warn('[BeaverTask] Failed to load persisted tasks.', error);");
    append("          return [];");
    append("        }");
    append("      };");
    append("      const saveTasks = () => {");
    append("        try {");
    append("          window.localStorage.setItem(storageKey, JSON.stringify(state.tasks));");
    append("        } catch (error) {");
    append("          console.warn('[BeaverTask] Failed to save tasks.', error);");
    append("        }");
    append("      };");
    append("      const generateId = () => `task_${Date.now().toString(36)}${Math.random().toString(36).slice(2, 8)}`;");
    append("      const ensureSelectedDate = () => {");
    append("        if (!(state.selectedDate instanceof Date)) {");
    append("          state.selectedDate = new Date();");
    append("        }");
    append("      };");
    append("      const formatReminder = (minutes) => {");
    append("        const value = Number(minutes);");
    append("        if (!Number.isFinite(value) || value <= 0) {");
    append("          return '';");
    append("        }");
    append("        if (value >= 1440) {");
    append("          const days = Math.floor(value / 1440);");
    append("          const template = strings.reminderDays || '%VALUE% day(s) before';");
    append("          return template.replace('%VALUE%', days.toString());");
    append("        }");
    append("        if (value >= 60) {");
    append("          const hours = Math.floor(value / 60);");
    append("          const template = strings.reminderHours || '%VALUE% hour(s) before';");
    append("          return template.replace('%VALUE%', hours.toString());");
    append("        }");
    append("        const template = strings.reminderMinutes || '%VALUE% min before';");
    append("        return template.replace('%VALUE%', value.toString());");
    append("      };");
    append("      const formatGroupTitle = (template, count) => {");
    append("        const placeholder = '%COUNT%';");
    append("        if (typeof template === 'string' && template.includes(placeholder)) {");
    append("          return template.replace(placeholder, String(count));");
    append("        }");
    append("        return `${template || ''} (${count})`;");
    append("      };");
    append("      const monthDisplay = document.getElementById('month-display');");
    append("      const calendarGrid = document.getElementById('calendar-grid');");
    append("      const detailsHeader = document.getElementById('details-header');");
    append("      const detailsContent = document.getElementById('details-content');");
    append("      const detailsAddBtn = document.getElementById('details-add-btn');");
    append("      const currentDayBadge = document.getElementById('current-day-badge');");
    append("      const tasklistSection = document.getElementById('tasklist-section');");
    append("      const tasklistContent = document.getElementById('tasklist-content');");
    append("      const tasklistHeaderEl = document.getElementById('tasklist-header');");
    append("      const taskModal = document.getElementById('task-modal');");
    append("      const taskForm = document.getElementById('task-form');");
    append("      const taskTitleInput = document.getElementById('task-title');");
    append("      const taskTagInput = document.getElementById('task-tag');");
    append("      const taskDescriptionInput = document.getElementById('task-description');");
    append("      const taskAllDayInput = document.getElementById('task-allday');");
    append("      const taskStartTimeInput = document.getElementById('task-start-time');");
    append("      const taskEndTimeInput = document.getElementById('task-end-time');");
    append("      const taskLocationInput = document.getElementById('task-location');");
    append("      const taskReminderInput = document.getElementById('task-reminder');");
    append("      const taskCancelBtn = document.getElementById('task-cancel');");
    append("      const taskModalDescription = document.getElementById('task-modal-description');");
    append("      const timeFields = document.getElementById('time-fields');");
    append("      const dateJumpModal = document.getElementById('date-jump-modal');");
    append("      const dateJumpForm = document.getElementById('date-jump-form');");
    append("      const jumpDateInput = document.getElementById('jump-date');");
    append("      const dateJumpCancelBtn = document.getElementById('date-jump-cancel');");
    append("      const searchBtn = document.getElementById('search-btn');");
    append("      const calendarIcon = document.getElementById('calendar-icon');");
    append("      const tasklistIcon = document.getElementById('tasklist-icon');");
    append("      const prevMonthBtn = document.getElementById('prev-month');");
    append("      const nextMonthBtn = document.getElementById('next-month');");
    append("      const renderCalendar = () => {");
    append("        if (!calendarGrid) {");
    append("          return;");
    append("        }");
    append("        calendarGrid.innerHTML = '';");
    append("        if (monthDisplay) {");
    append("          monthDisplay.textContent = monthNamesShort[state.currentMonth] || monthNamesShort[0] || '';");
    append("        }");
    append("        const firstDay = new Date(state.currentYear, state.currentMonth, 1);");
    append("        const lastDay = new Date(state.currentYear, state.currentMonth + 1, 0);");
    append("        const startDay = firstDay.getDay();");
    append("        const totalDays = lastDay.getDate();");
    append("        const taskMap = state.tasks.reduce((map, task) => {");
    append("          if (task.dueDate) {");
    append("            if (!map.has(task.dueDate)) {");
    append("              map.set(task.dueDate, []);");
    append("            }");
    append("            map.get(task.dueDate).push(task);");
    append("          }");
    append("          return map;");
    append("        }, new Map());");
    append("        const createCell = (date, inactive) => {");
    append("          const cell = document.createElement('button');");
    append("          cell.type = 'button';");
    append("          cell.className = 'calendar-cell';");
    append("          if (inactive) {");
    append("            cell.classList.add('inactive');");
    append("          }");
    append("          const dateKey = getDateKey(date);");
    append("          const today = new Date();");
    append("          if (dateKey === getDateKey(today)) {");
    append("            cell.classList.add('today');");
    append("          }");
    append("          if (state.selectedDate && getDateKey(state.selectedDate) === dateKey) {");
    append("            cell.classList.add('selected');");
    append("          }");
    append("          if (taskMap.has(dateKey)) {");
    append("            cell.classList.add('has-events');");
    append("          }");
    append("          const label = document.createElement('span');");
    append("          label.className = 'cell-date';");
    append("          label.textContent = String(date.getDate());");
    append("          cell.appendChild(label);");
    append("          cell.addEventListener('click', () => {");
    append("            state.selectedDate = date;");
    append("            renderCalendar();");
    append("            renderDetails();");
    append("          });");
    append("          return cell;");
    append("        };");
    append("        for (let i = 0; i < startDay; i += 1) {");
    append("          const prevDate = new Date(state.currentYear, state.currentMonth, -startDay + i + 1);");
    append("          calendarGrid.appendChild(createCell(prevDate, true));");
    append("        }");
    append("        for (let day = 1; day <= totalDays; day += 1) {");
    append("          const currentDate = new Date(state.currentYear, state.currentMonth, day);");
    append("          calendarGrid.appendChild(createCell(currentDate, false));");
    append("        }");
    append("        const cellsCount = calendarGrid.children.length;");
    append("        const remaining = Math.ceil(cellsCount / 7) * 7 - cellsCount;");
    append("        for (let offset = 1; offset <= remaining; offset += 1) {");
    append("          const nextDate = new Date(state.currentYear, state.currentMonth + 1, offset);");
    append("          calendarGrid.appendChild(createCell(nextDate, true));");
    append("        }");
    append("      };");
    append("      const renderDetails = () => {");
    append("        if (!detailsHeader || !detailsContent) {");
    append("          return;");
    append("        }");
    append("        ensureSelectedDate();");
    append("        const dateKey = getDateKey(state.selectedDate);");
    append("        const day = state.selectedDate.getDate();");
    append("        const monthLabel = monthNamesDetail[state.selectedDate.getMonth()] || monthNamesDetail[0] || '';");
    append("        detailsHeader.textContent = `${day} ${monthLabel}`;");
    append("        const tasksForDate = state.tasks.filter((task) => task.dueDate === dateKey);");
    append("        if (!tasksForDate.length) {");
    append("          detailsContent.innerHTML = `<div class=\"empty-state\"><div class=\"empty-icon\">☺</div><div class=\"empty-text\">${escapeHtml(strings.emptyDetails || '')}</div></div>`;");
    append("          return;");
    append("        }");
    append("        const html = tasksForDate.map((task) => {");
    append("          const id = escapeHtml(task.id || '');");
    append("          const title = escapeHtml(task.title || '');");
    append("          const color = sanitizeColor(task.color || '');");
    append("          const metaParts = [];");
    append("          if (task.allDay) {");
    append("            metaParts.push(escapeHtml(strings.metaAllDay || 'All day'));");
    append("          } else if (task.startTime) {");
    append("            const range = task.endTime ? `${task.startTime} - ${task.endTime}` : task.startTime;");
    append("            metaParts.push(`🕐 ${escapeHtml(range)}`);");
    append("          }");
    append("          if (task.location) {");
    append("            metaParts.push(`📍 ${escapeHtml(task.location)}`);");
    append("          }");
    append("          if (task.reminderMinutes && Number(task.reminderMinutes) > 0) {");
    append("            metaParts.push(`🔔 ${escapeHtml(formatReminder(task.reminderMinutes))}`);");
    append("          }");
    append("          if (task.tag) {");
    append("            metaParts.push(escapeHtml(task.tag));");
    append("          }");
    append("          if (task.description) {");
    append("            metaParts.push(escapeHtml(task.description));");
    append("          }");
    append("          const metaHtml = metaParts.length ? `<div class=\"task-meta\">${metaParts.join(' • ')}</div>` : '';");
    append("          const colorDot = color ? `<div class=\"task-color-dot\" style=\"background: ${color};\"></div>` : '';");
    append("          return `<div class=\"task-item\"><button type=\"button\" class=\"task-checkbox${task.completed ? ' completed' : ''}\" data-id=\"${id}\" aria-pressed=\"${task.completed ? 'true' : 'false'}\"></button><div class=\"task-info\"><div class=\"task-title\">${colorDot}<span>${title}</span></div>${metaHtml}</div><div class=\"task-actions\"><button type=\"button\" class=\"task-action-btn delete\" data-action=\"delete\" data-id=\"${id}\">×</button></div></div>`;");
    append("        }).join('');");
    append("        detailsContent.innerHTML = `<div class=\"task-list\">${html}</div>`;");
    append("        detailsContent.querySelectorAll('.task-checkbox').forEach((checkbox) => {");
    append("          checkbox.addEventListener('click', async (event) => {");
    append("            const id = event.currentTarget.getAttribute('data-id');");
    append("            const task = state.tasks.find((entry) => entry.id === id);");
    append("            if (task) {");
    append("              await updateTask(id, { completed: !task.completed });");
    append("            }");
    append("          });");
    append("        });");
    append("        detailsContent.querySelectorAll('.task-action-btn.delete').forEach((button) => {");
    append("          button.addEventListener('click', async (event) => {");
    append("            event.stopPropagation();");
    append("            const id = event.currentTarget.getAttribute('data-id');");
    append("            if (!id) {");
    append("              return;");
    append("            }");
    append("            const message = strings.confirmDelete || 'Delete this task?';");
    append("            if (window.confirm(message)) {");
    append("              await deleteTask(id);");
    append("            }");
    append("          });");
    append("        });");
    append("      };");
    append("      const changeMonth = (offset) => {");
    append("        state.currentMonth += offset;");
    append("        if (state.currentMonth < 0) {");
    append("          state.currentMonth = 11;");
    append("          state.currentYear -= 1;");
    append("        } else if (state.currentMonth > 11) {");
    append("          state.currentMonth = 0;");
    append("          state.currentYear += 1;");
    append("        }");
    append("        renderCalendar();");
    append("        renderDetails();");
    append("      };");
    append("      const updateCurrentDayBadge = () => {");
    append("        if (!currentDayBadge) {");
    append("          return;");
    append("        }");
    append("        const today = new Date();");
    append("        currentDayBadge.textContent = String(today.getDate());");
    append("      };");
    append("      const renderTaskListItem = (task) => {");
    append("        const id = escapeHtml(task.id || '');");
    append("        const title = escapeHtml(task.title || '');");
    append("        const due = parseDateValue(task.dueDate);");
    append("        const formattedDate = due ? due.toLocaleDateString(locale, { month: 'short', day: 'numeric', year: 'numeric' }) : (strings.noDueDate || 'No due date');");
    append("        const tagDisplay = task.tag ? ` • ${escapeHtml(task.tag)}` : '';");
    append("        return `<div class=\"task-item\"><button type=\"button\" class=\"task-checkbox${task.completed ? ' completed' : ''}\" data-id=\"${id}\" aria-pressed=\"${task.completed ? 'true' : 'false'}\"></button><div class=\"task-info\"><div class=\"task-title\">${title}</div><div class=\"task-meta\">${escapeHtml(formattedDate)}${tagDisplay}</div></div><div class=\"task-actions\"><button type=\"button\" class=\"task-action-btn delete\" data-id=\"${id}\">🗑️</button></div></div>`;");
    append("      };");
    append("      const renderTaskList = () => {");
    append("        if (!tasklistContent) {");
    append("          return;");
    append("        }");
    append("        if (!state.tasks.length) {");
    append("          tasklistContent.innerHTML = `<div class=\"tasklist-empty\"><div class=\"empty-icon\">📋</div><div class=\"empty-text\">${escapeHtml(strings.taskListEmpty || '')}</div></div>`;");
    append("          return;");
    append("        }");
    append("        const today = new Date();");
    append("        today.setHours(0, 0, 0, 0);");
    append("        const incomplete = state.tasks.filter((task) => !task.completed);");
    append("        const completed = state.tasks.filter((task) => task.completed);");
    append("        const toDateValue = (value) => {");
    append("          const parsed = parseDateValue(value);");
    append("          return parsed ? parsed.getTime() : null;");
    append("        };");
    append("        const upcoming = incomplete.filter((task) => {");
    append("          const time = toDateValue(task.dueDate);");
    append("          return time !== null && time >= today.getTime();");
    append("        }).sort((a, b) => (toDateValue(a.dueDate) || 0) - (toDateValue(b.dueDate) || 0));");
    append("        const overdue = incomplete.filter((task) => {");
    append("          const time = toDateValue(task.dueDate);");
    append("          return time !== null && time < today.getTime();");
    append("        }).sort((a, b) => (toDateValue(a.dueDate) || 0) - (toDateValue(b.dueDate) || 0));");
    append("        const noDueDate = incomplete.filter((task) => !task.dueDate);");
    append("        let html = '';");
    append("        if (overdue.length) {");
    append("          html += `<div class=\"tasklist-group\"><div class=\"tasklist-group-title\">${escapeHtml(formatGroupTitle(strings.groupOverdue || 'Overdue', overdue.length))}</div><div class=\"task-list\">${overdue.map(renderTaskListItem).join('')}</div></div>`;");
    append("        }");
    append("        if (upcoming.length) {");
    append("          html += `<div class=\"tasklist-group\"><div class=\"tasklist-group-title\">${escapeHtml(formatGroupTitle(strings.groupUpcoming || 'Upcoming', upcoming.length))}</div><div class=\"task-list\">${upcoming.map(renderTaskListItem).join('')}</div></div>`;");
    append("        }");
    append("        if (noDueDate.length) {");
    append("          html += `<div class=\"tasklist-group\"><div class=\"tasklist-group-title\">${escapeHtml(formatGroupTitle(strings.groupNoDueDate || 'No Due Date', noDueDate.length))}</div><div class=\"task-list\">${noDueDate.map(renderTaskListItem).join('')}</div></div>`;");
    append("        }");
    append("        if (completed.length) {");
    append("          html += `<div class=\"tasklist-group\"><div class=\"tasklist-group-title\">${escapeHtml(formatGroupTitle(strings.groupCompleted || 'Completed', completed.length))}</div><div class=\"task-list\">${completed.map(renderTaskListItem).join('')}</div></div>`;");
    append("        }");
    append("        tasklistContent.innerHTML = html;");
    append("        tasklistContent.querySelectorAll('.task-checkbox').forEach((checkbox) => {");
    append("          checkbox.addEventListener('click', async (event) => {");
    append("            const id = event.currentTarget.getAttribute('data-id');");
    append("            const task = state.tasks.find((entry) => entry.id === id);");
    append("            if (task) {");
    append("              await updateTask(id, { completed: !task.completed });");
    append("            }");
    append("          });");
    append("        });");
    append("        tasklistContent.querySelectorAll('.task-action-btn.delete').forEach((button) => {");
    append("          button.addEventListener('click', async (event) => {");
    append("            const id = event.currentTarget.getAttribute('data-id');");
    append("            if (!id) {");
    append("              return;");
    append("            }");
    append("            const message = strings.confirmDelete || 'Delete this task?';");
    append("            if (window.confirm(message)) {");
    append("              await deleteTask(id);");
    append("            }");
    append("          });");
    append("        });");
    append("      };");
    append("      const rerenderAll = () => {");
    append("        renderCalendar();");
    append("        renderDetails();");
    append("        renderTaskList();");
    append("      };");
    append("      const addTask = async (taskData) => {");
    append("        const newTask = Object.assign({ id: generateId(), completed: false }, taskData);");
    append("        state.tasks.push(newTask);");
    append("        saveTasks();");
    append("        rerenderAll();");
    append("        return true;");
    append("      };");
    append("      const updateTask = async (id, updates) => {");
    append("        const index = state.tasks.findIndex((task) => task.id === id);");
    append("        if (index === -1) {");
    append("          return false;");
    append("        }");
    append("        state.tasks[index] = Object.assign({}, state.tasks[index], updates);");
    append("        saveTasks();");
    append("        rerenderAll();");
    append("        return true;");
    append("      };");
    append("      const deleteTask = async (id) => {");
    append("        const originalLength = state.tasks.length;");
    append("        state.tasks = state.tasks.filter((task) => task.id !== id);");
    append("        if (state.tasks.length === originalLength) {");
    append("          return false;");
    append("        }");
    append("        saveTasks();");
    append("        rerenderAll();");
    append("        return true;");
    append("      };");
    append("      const openTaskModal = () => {");
    append("        if (!taskModal) {");
    append("          return;");
    append("        }");
    append("        ensureSelectedDate();");
    append("        const day = state.selectedDate.getDate();");
    append("        const month = monthNamesFull[state.selectedDate.getMonth()] || monthNamesFull[0] || '';");
    append("        const year = state.selectedDate.getFullYear();");
    append("        const template = strings.modalDateTemplate || 'Add an event for %DATE%.';");
    append("        const description = template.includes('%DATE%') ? template.replace('%DATE%', `${day} ${month} ${year}`) : `${template} ${day} ${month} ${year}`;");
    append("        if (taskModalDescription) {");
    append("          taskModalDescription.textContent = description;");
    append("        }");
    append("        if (taskAllDayInput && timeFields) {");
    append("          taskAllDayInput.checked = false;");
    append("          timeFields.style.display = 'grid';");
    append("        }");
    append("        taskModal.removeAttribute('hidden');");
    append("        window.requestAnimationFrame(() => {");
    append("          taskTitleInput && taskTitleInput.focus();");
    append("        });");
    append("      };");
    append("      const closeTaskModal = () => {");
    append("        if (!taskModal) {");
    append("          return;");
    append("        }");
    append("        taskModal.setAttribute('hidden', '');");
    append("        taskForm && taskForm.reset();");
    append("      };");
    append("      const openDateJumpModal = () => {");
    append("        if (!dateJumpModal) {");
    append("          return;");
    append("        }");
    append("        ensureSelectedDate();");
    append("        if (jumpDateInput) {");
    append("          jumpDateInput.value = getDateKey(state.selectedDate);");
    append("        }");
    append("        dateJumpModal.removeAttribute('hidden');");
    append("        window.requestAnimationFrame(() => {");
    append("          jumpDateInput && jumpDateInput.focus();");
    append("        });");
    append("      };");
    append("      const closeDateJumpModal = () => {");
    append("        if (!dateJumpModal) {");
    append("          return;");
    append("        }");
    append("        dateJumpModal.setAttribute('hidden', '');");
    append("        dateJumpForm && dateJumpForm.reset();");
    append("      };");
    append("      taskAllDayInput && taskAllDayInput.addEventListener('change', (event) => {");
    append("        if (!timeFields) {");
    append("          return;");
    append("        }");
    append("        timeFields.style.display = event.target.checked ? 'none' : 'grid';");
    append("      });");
    append("      detailsAddBtn && detailsAddBtn.addEventListener('click', openTaskModal);");
    append("      taskCancelBtn && taskCancelBtn.addEventListener('click', closeTaskModal);");
    append("      taskModal && taskModal.addEventListener('click', (event) => {");
    append("        if (event.target === taskModal) {");
    append("          closeTaskModal();");
    append("        }");
    append("      });");
    append("      taskForm && taskForm.addEventListener('submit', async (event) => {");
    append("        event.preventDefault();");
    append("        const title = taskTitleInput ? taskTitleInput.value.trim() : '';");
    append("        if (!title) {");
    append("          taskTitleInput && taskTitleInput.focus();");
    append("          return;");
    append("        }");
    append("        ensureSelectedDate();");
    append("        const allDay = taskAllDayInput ? taskAllDayInput.checked : false;");
    append("        const startTime = allDay ? '' : (taskStartTimeInput ? taskStartTimeInput.value : '');");
    append("        const endTime = allDay ? '' : (taskEndTimeInput ? taskEndTimeInput.value : '');");
    append("        const locationValue = taskLocationInput ? taskLocationInput.value.trim() : '';");
    append("        const reminderValue = taskReminderInput ? Number.parseInt(taskReminderInput.value, 10) : 0;");
    append("        const colorInput = root.querySelector('input[name=\"color\"]:checked');");
    append("        const colorValue = sanitizeColor(colorInput ? colorInput.value : '#f89422');");
    append("        await addTask({");
    append("          title,");
    append("          dueDate: getDateKey(state.selectedDate),");
    append("          description: taskDescriptionInput ? taskDescriptionInput.value.trim() : '',");
    append("          tag: taskTagInput ? taskTagInput.value.trim() : '',");
    append("          startTime,");
    append("          endTime,");
    append("          allDay,");
    append("          location: locationValue,");
    append("          reminderMinutes: Number.isFinite(reminderValue) && reminderValue > 0 ? reminderValue : 0,");
    append("          color: colorValue,");
    append("          completed: false");
    append("        });");
    append("        closeTaskModal();");
    append("      });");
    append("      searchBtn && searchBtn.addEventListener('click', openDateJumpModal);");
    append("      dateJumpCancelBtn && dateJumpCancelBtn.addEventListener('click', closeDateJumpModal);");
    append("      dateJumpModal && dateJumpModal.addEventListener('click', (event) => {");
    append("        if (event.target === dateJumpModal) {");
    append("          closeDateJumpModal();");
    append("        }");
    append("      });");
    append("      dateJumpForm && dateJumpForm.addEventListener('submit', (event) => {");
    append("        event.preventDefault();");
    append("        if (!jumpDateInput || !jumpDateInput.value) {");
    append("          return;");
    append("        }");
    append("        const targetDate = parseDateValue(jumpDateInput.value);");
    append("        if (!targetDate) {");
    append("          return;");
    append("        }");
    append("        state.currentYear = targetDate.getFullYear();");
    append("        state.currentMonth = targetDate.getMonth();");
    append("        state.selectedDate = targetDate;");
    append("        rerenderAll();");
    append("        closeDateJumpModal();");
    append("      });");
    append("      const showCalendarView = () => {");
    append("        const calendarSection = document.querySelector('.calendar-section');");
    append("        const detailsPanel = document.querySelector('.details-panel');");
    append("        if (calendarSection) {");
    append("          calendarSection.style.display = 'flex';");
    append("        }");
    append("        if (detailsPanel) {");
    append("          detailsPanel.style.display = 'flex';");
    append("        }");
    append("        if (tasklistSection) {");
    append("          tasklistSection.classList.remove('active');");
    append("        }");
    append("        calendarIcon && calendarIcon.classList.add('active');");
    append("        tasklistIcon && tasklistIcon.classList.remove('active');");
    append("      };");
    append("      const showTaskListView = () => {");
    append("        const calendarSection = document.querySelector('.calendar-section');");
    append("        const detailsPanel = document.querySelector('.details-panel');");
    append("        if (calendarSection) {");
    append("          calendarSection.style.display = 'none';");
    append("        }");
    append("        if (detailsPanel) {");
    append("          detailsPanel.style.display = 'none';");
    append("        }");
    append("        if (tasklistSection) {");
    append("          tasklistSection.classList.add('active');");
    append("        }");
    append("        calendarIcon && calendarIcon.classList.remove('active');");
    append("        tasklistIcon && tasklistIcon.classList.add('active');");
    append("        renderTaskList();");
    append("      };");
    append("      calendarIcon && calendarIcon.addEventListener('click', showCalendarView);");
    append("      tasklistIcon && tasklistIcon.addEventListener('click', showTaskListView);");
    append("      prevMonthBtn && prevMonthBtn.addEventListener('click', () => changeMonth(-1));");
    append("      nextMonthBtn && nextMonthBtn.addEventListener('click', () => changeMonth(1));");
    append("      document.addEventListener('keydown', (event) => {");
    append("        if (event.key === 'Escape') {");
    append("          if (taskModal && !taskModal.hasAttribute('hidden')) {");
    append("            closeTaskModal();");
    append("          } else if (dateJumpModal && !dateJumpModal.hasAttribute('hidden')) {");
    append("            closeDateJumpModal();");
    append("          }");
    append("        }");
    append("      });");
    append("      window.addEventListener('storage', (event) => {");
    append("        if (event.key === storageKey) {");
    append("          state.tasks = loadTasks();");
    append("          rerenderAll();");
    append("        }");
    append("      });");
    append("      const init = () => {");
    append("        state.tasks = loadTasks();");
    append("        state.selectedDate = new Date();");
    append("        state.currentYear = state.selectedDate.getFullYear();");
    append("        state.currentMonth = state.selectedDate.getMonth();");
    append("        updateCurrentDayBadge();");
    append("        if (tasklistHeaderEl && strings.taskListHeader) {");
    append("          tasklistHeaderEl.textContent = strings.taskListHeader;");
    append("        }");
    append("        rerenderAll();");
    append("      };");
    append("      init();");
    append("    })();");
    append("  </script>");
    append("</body>");
    append("</html>");

    return html.str();
}
