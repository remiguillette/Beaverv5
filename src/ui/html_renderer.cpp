#include "ui/html_renderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

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
    append("        <a class=\"system-header__back\" href=\"" + menu_href + "\">");
    append("          <span class=\"system-header__back-icon\" aria-hidden=\"true\">&larr;</span>");
    append("          <span class=\"system-header__back-label\">" + html_escape(back_to_menu) + "</span>");
    append("        </a>");
    append("        <h1 class=\"system-header__title\">" + html_escape(beaversystem_label) + "</h1>");
    append("        <nav class=\"lang-toggle\" role=\"group\" aria-label=\"" + html_escape(language_label) + "\">");
    append(language_toggle_button("FR", beaversystem_french_href, switch_to_french, language == Language::French));
    append(language_toggle_button("EN", beaversystem_english_href, switch_to_english, language == Language::English));
    append("        </nav>");
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
    append("        setText(wsLastMessageEl, lastMessage ? lastMessage : strings.noTelemetry);");
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
