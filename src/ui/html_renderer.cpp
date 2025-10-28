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
     "0022"},
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
    const std::string clear_label = translations.translate("Clear", language);
    const std::string description = translations.translate("BeaverPhone description", language);
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
             << "\" data-extension-value=\"" << contact.extension
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

          if (digits.length === 0) {
            displayWrapper.classList.add('is-empty');
            if (lastRendered !== placeholder) {
              displayValue.textContent = placeholder;
              lastRendered = placeholder;
            }
          } else {
            displayWrapper.classList.remove('is-empty');
            const joined = formatDigits(digits);
            if (joined !== lastRendered) {
              displayValue.textContent = joined;
              lastRendered = joined;
            }
          }

          if (callButton) {
            callButton.disabled = !isCompleteLength(digits.length);
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
          if (!digits.length) {
            return;
          }
          digits.length = 0;
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
            window.setTimeout(() => {
              window.dispatchEvent(new CustomEvent('beaverphone:call', { detail: { digits: payload } }));
            }, 0);
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
