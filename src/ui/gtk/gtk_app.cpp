#include "ui/gtk/gtk_app.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include <webkit2/webkit2.h>
#if WEBKIT_CHECK_VERSION(2, 40, 0)
#include <JavaScriptCore/JavaScript.h>
#endif
#include <gdk/gdk.h>
#include <glib.h>

#include "core/cctv_config.h"

#if defined(HAVE_GSTREAMER)
#include <gst/gst.h>
#endif

GtkApp::GtkApp(AppManager& manager) : manager_(manager) {}

int GtkApp::run(int argc, char** argv) {
#if defined(HAVE_GSTREAMER)
    ensure_gstreamer_initialized();
#endif
    GtkApplication* application =
        gtk_application_new("com.beaver.kiosk", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(application, "activate", G_CALLBACK(GtkApp::on_activate), this);
#if defined(HAVE_GSTREAMER)
    g_signal_connect(application, "shutdown", G_CALLBACK(GtkApp::on_application_shutdown), this);
#endif

    int status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}

void GtkApp::on_activate(GtkApplication* application, gpointer user_data) {
    auto* self = static_cast<GtkApp*>(user_data);
    self->build_ui(application);
}

namespace {
constexpr const char kRemoteBackButtonScript[] = R"JS((() => {
  const WRAPPER_ID = 'beaverRemoteMenuWrapper';
  const BUTTON_ID = 'beaverRemoteMenuButton';
  if (document.getElementById(WRAPPER_ID)) {
    return;
  }

  const wrapper = document.createElement('div');
  wrapper.id = WRAPPER_ID;
  Object.assign(wrapper.style, {
    position: 'fixed',
    top: '20px',
    left: '20px',
    zIndex: 9999,
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    padding: '4px',
    borderRadius: '16px',
    background: 'transparent'
  });

  const button = document.createElement('button');
  button.type = 'button';
  button.id = BUTTON_ID;
  button.setAttribute('aria-label', 'Return to menu');
  Object.assign(button.style, {
    display: 'inline-flex',
    alignItems: 'center',
    gap: '0.5rem',
    background: 'rgba(9, 12, 20, 0.85)',
    color: '#f2f2f7',
    fontWeight: '600',
    fontSize: '15px',
    padding: '10px 16px',
    border: '1px solid rgba(255, 255, 255, 0.18)',
    borderRadius: '14px',
    cursor: 'pointer',
    boxShadow: '0 8px 24px rgba(0, 0, 0, 0.35)',
    backdropFilter: 'blur(12px)',
    transition: 'opacity 160ms ease, transform 160ms ease',
    transform: 'translateY(-6px)'
  });

  const ns = 'http://www.w3.org/2000/svg';
  const icon = document.createElementNS(ns, 'svg');
  icon.setAttribute('aria-hidden', 'true');
  icon.setAttribute('width', '20');
  icon.setAttribute('height', '20');
  icon.setAttribute('viewBox', '0 0 24 24');
  icon.setAttribute('fill', 'none');
  icon.setAttribute('stroke', 'currentColor');
  icon.setAttribute('stroke-width', '2');
  icon.setAttribute('stroke-linecap', 'round');
  icon.setAttribute('stroke-linejoin', 'round');

  const line = document.createElementNS(ns, 'line');
  line.setAttribute('x1', '19');
  line.setAttribute('y1', '12');
  line.setAttribute('x2', '5');
  line.setAttribute('y2', '12');

  const polyline = document.createElementNS(ns, 'polyline');
  polyline.setAttribute('points', '12 19 5 12 12 5');

  icon.appendChild(line);
  icon.appendChild(polyline);

  const text = document.createElement('span');
  text.textContent = 'Menu';

  button.appendChild(icon);
  button.appendChild(text);

  const setInteractive = () => {
    button.style.opacity = '1';
    button.style.pointerEvents = 'auto';
    button.style.transform = 'translateY(0)';
  };

  const setPassive = () => {
    button.style.opacity = '0';
    button.style.pointerEvents = 'none';
    button.style.transform = 'translateY(-6px)';
  };

  wrapper.addEventListener('mouseenter', setInteractive);
  wrapper.addEventListener('mouseleave', () => {
    if (!button.matches(':focus')) {
      setPassive();
    }
  });
  wrapper.addEventListener('touchstart', () => {
    setInteractive();
    window.setTimeout(() => {
      if (!button.matches(':focus')) {
        setPassive();
      }
    }, 2500);
  }, { passive: true });
  button.addEventListener('focus', setInteractive);
  button.addEventListener('blur', setPassive);

  button.addEventListener('click', () => {
    try {
      const handler = window.webkit && window.webkit.messageHandlers &&
        window.webkit.messageHandlers.beaverkiosk;
      if (handler && typeof handler.postMessage === 'function') {
        handler.postMessage('go-home');
      } else {
        window.history.back();
      }
    } catch (error) {
      window.history.back();
    }
  });

  document.body.appendChild(wrapper);
  setPassive();
})();)JS";

constexpr const char kRemoveRemoteBackButtonScript[] = R"JS((() => {
  const wrapper = document.getElementById('beaverRemoteMenuWrapper');
  if (wrapper && wrapper.parentNode) {
    wrapper.parentNode.removeChild(wrapper);
  }
})();)JS";

std::string build_base_uri() {
    namespace fs = std::filesystem;
    fs::path public_dir = fs::current_path() / "public";
    static bool reported_missing_public_dir = false;
    static bool reported_public_dir = false;

    if (!fs::exists(public_dir)) {
        if (!reported_missing_public_dir) {
            g_warning("GtkApp could not find public assets directory: %s",
                      public_dir.string().c_str());
            reported_missing_public_dir = true;
        }
    } else if (!reported_public_dir) {
        g_message("GtkApp using public assets directory: %s", public_dir.string().c_str());
        reported_public_dir = true;
    }

    std::string uri = "file://" + public_dir.string();
    if (!uri.empty() && uri.back() != '/') {
        uri.push_back('/');
    }
    return uri;
}

std::string strip_trailing_slashes(std::string path) {
    if (path.size() <= 1) {
        return path;
    }

    std::size_t new_size = path.size();
    while (new_size > 1 && path[new_size - 1] == '/') {
        --new_size;
    }

    if (new_size != path.size()) {
        path.resize(new_size);
    }
    return path;
}

void set_widget_visible(GtkWidget* widget, bool visible) {
    if (widget == nullptr) {
        return;
    }

    if (!GTK_IS_WIDGET(widget)) {
        g_warning("GtkApp attempted to update visibility of a non-widget pointer. Ignoring.");
        return;
    }
#if GTK_MAJOR_VERSION >= 4
    gtk_widget_set_visible(widget, visible);
#else
    if (visible) {
        gtk_widget_show(widget);
    } else {
        gtk_widget_hide(widget);
    }
#endif
}

std::string normalize_navigation_path(const std::string& path) {
    if (path.empty()) {
        return path;
    }

    namespace fs = std::filesystem;
    fs::path public_dir = fs::current_path() / "public";
    std::string public_dir_string = public_dir.string();

    if (!public_dir_string.empty() && path.rfind(public_dir_string, 0) == 0) {
        std::string remainder = path.substr(public_dir_string.size());
        if (remainder.empty()) {
            remainder = "/";
        }
        if (!remainder.empty() && remainder.front() != '/') {
            remainder.insert(remainder.begin(), '/');
        }
        return strip_trailing_slashes(remainder);
    }

    if (path.front() != '/') {
        return strip_trailing_slashes(std::string("/") + path);
    }

    return strip_trailing_slashes(path);
}

Language language_from_query(const std::string& query, Language fallback) {
    Language language = fallback;
    std::stringstream query_stream(query);
    std::string parameter;
    while (std::getline(query_stream, parameter, '&')) {
        const std::size_t separator_position = parameter.find('=');
        if (separator_position == std::string::npos) {
            continue;
        }

        std::string key = parameter.substr(0, separator_position);
        std::string value = parameter.substr(separator_position + 1);
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (key != "lang") {
            continue;
        }

        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (value == "en") {
            language = Language::English;
        } else if (value == "fr") {
            language = Language::French;
        }
        break;
    }

    return language;
}

const char* policy_decision_type_to_string(WebKitPolicyDecisionType decision_type) {
    switch (decision_type) {
        case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
            return "navigation-action";
        case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
            return "new-window-action";
        case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:
            return "response";
        default:
            return "unknown";
    }
}

const char* navigation_type_to_string(WebKitNavigationType navigation_type) {
    switch (navigation_type) {
        case WEBKIT_NAVIGATION_TYPE_LINK_CLICKED:
            return "link-clicked";
        case WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED:
            return "form-submitted";
        case WEBKIT_NAVIGATION_TYPE_BACK_FORWARD:
            return "back-forward";
        case WEBKIT_NAVIGATION_TYPE_RELOAD:
            return "reload";
        case WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED:
            return "form-resubmitted";
        case WEBKIT_NAVIGATION_TYPE_OTHER:
        default:
            return "other";
    }
}

#if defined(WEBKIT_CONSOLE_MESSAGE_LEVEL_INFO)
const char* console_message_level_to_string(WebKitConsoleMessageLevel level) {
    switch (level) {
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_INFO:
            return "info";
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_LOG:
            return "log";
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_WARNING:
            return "warning";
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_ERROR:
            return "error";
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_DEBUG:
            return "debug";
        default:
            return "unknown";
    }
}

const char* console_message_source_to_string(WebKitConsoleMessageSource source) {
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_HTML
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_HTML) {
        return "html";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_XML
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_XML) {
        return "xml";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_JAVASCRIPT
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_JAVASCRIPT) {
        return "javascript";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_NETWORK
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_NETWORK) {
        return "network";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_CONSOLE_API
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_CONSOLE_API) {
        return "console-api";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_STORAGE
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_STORAGE) {
        return "storage";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_APP_CACHE
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_APP_CACHE) {
        return "app-cache";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_RENDERING
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_RENDERING) {
        return "rendering";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_CSS
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_CSS) {
        return "css";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_SECURITY
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_SECURITY) {
        return "security";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_CONTENT_BLOCKER
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_CONTENT_BLOCKER) {
        return "content-blocker";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_MEDIA
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_MEDIA) {
        return "media";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_WEBRTC
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_WEBRTC) {
        return "webrtc";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_ITP
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_ITP) {
        return "itp";
    }
#endif
#ifdef WEBKIT_CONSOLE_MESSAGE_SOURCE_OTHER
    if (source == WEBKIT_CONSOLE_MESSAGE_SOURCE_OTHER) {
        return "other";
    }
#endif
    return "unknown";
}
#endif  // defined(WEBKIT_CONSOLE_MESSAGE_LEVEL_INFO)
}  // namespace

void GtkApp::build_ui(GtkApplication* application) {
    GtkWidget* window = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(window), "BeaverKiosk");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

    auto configure_webview = []() -> GtkWidget* {
        static WebKitWebContext* shared_context = []() {
            WebKitWebContext* context = webkit_web_context_new();
            webkit_web_context_set_cache_model(context, WEBKIT_CACHE_MODEL_WEB_BROWSER);
            return context;
        }();

        WebKitWebContext* context = WEBKIT_WEB_CONTEXT(g_object_ref(shared_context));
        GtkWidget* view = GTK_WIDGET(webkit_web_view_new_with_context(context));
        g_object_unref(context);

        WebKitSettings* settings = webkit_settings_new();
        webkit_settings_set_hardware_acceleration_policy(settings,
                                                        WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);
        webkit_settings_set_enable_smooth_scrolling(settings, FALSE);
        webkit_settings_set_enable_webaudio(settings, FALSE);
#if WEBKIT_CHECK_VERSION(2, 22, 0)
        webkit_settings_set_enable_media_stream(settings, FALSE);
#endif
#if WEBKIT_CHECK_VERSION(2, 28, 0)
        webkit_settings_set_enable_webrtc(settings, FALSE);
#endif
        webkit_web_view_set_settings(WEBKIT_WEB_VIEW(view), settings);
        g_object_unref(settings);

        GdkRGBA background = {0.06, 0.066, 0.094, 1.0};
        webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(view), &background);
        return view;
    };

    GtkWidget* webview = configure_webview();
    web_view_ = WEBKIT_WEB_VIEW(webview);

    WebKitUserContentManager* content_manager =
        webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));
    if (content_manager != nullptr) {
        if (!webkit_user_content_manager_register_script_message_handler(content_manager,
                                                                         "beaverkiosk")) {
            g_warning("GtkApp failed to register script message handler 'beaverkiosk'.");
        }
        g_signal_connect(content_manager, "script-message-received::beaverkiosk",
                         G_CALLBACK(GtkApp::on_script_message_received), this);
    }

    g_signal_connect(webview, "decide-policy", G_CALLBACK(GtkApp::on_decide_policy), this);
    g_signal_connect(webview, "load-changed", G_CALLBACK(GtkApp::on_load_changed), this);
#if defined(WEBKIT_CONSOLE_MESSAGE_LEVEL_INFO)
    g_signal_connect(webview, "console-message-sent", G_CALLBACK(GtkApp::on_console_message), this);
#endif
    GtkWidget* overlay = gtk_overlay_new();
    overlay_root_ = overlay;
#if GTK_MAJOR_VERSION >= 4
    gtk_overlay_set_child(GTK_OVERLAY(overlay), webview);
    gtk_window_set_child(GTK_WINDOW(window), overlay);
#else
    gtk_container_add(GTK_CONTAINER(overlay), webview);
    gtk_container_add(GTK_CONTAINER(window), overlay);
#endif

#if defined(HAVE_GSTREAMER)
    ensure_camera_overlay();
#endif

    Language initial_language = manager_.get_default_language();
    g_message("GtkApp building UI with initial language: %s",
              language_to_string(initial_language));
    load_language(WEBKIT_WEB_VIEW(webview), initial_language);

#if GTK_MAJOR_VERSION >= 4
    gtk_window_present(GTK_WINDOW(window));
#else
    gtk_widget_show_all(window);
#endif
}

gboolean GtkApp::on_decide_policy(WebKitWebView* web_view, WebKitPolicyDecision* decision,
                                  WebKitPolicyDecisionType decision_type, gpointer user_data) {
    g_message("GtkApp policy decision received. type=%s (%d)",
              policy_decision_type_to_string(decision_type), static_cast<int>(decision_type));

    if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
        g_message("GtkApp allowing non-navigation policy decision to proceed normally.");
        return FALSE;
    }

    auto* self = static_cast<GtkApp*>(user_data);
    if (self == nullptr) {
        return FALSE;
    }

    auto* navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
    WebKitNavigationAction* action =
        webkit_navigation_policy_decision_get_navigation_action(navigation_decision);
    WebKitURIRequest* request = webkit_navigation_action_get_request(action);
    const gchar* uri = webkit_uri_request_get_uri(request);
    if (uri == nullptr) {
        return FALSE;
    }

    const WebKitNavigationType navigation_type =
        webkit_navigation_action_get_navigation_type(action);
    const gboolean is_user_gesture = webkit_navigation_action_is_user_gesture(action);
    // When we call webkit_web_view_load_html the web view performs an internal
    // navigation to the provided base URI. Those navigations are reported with
    // type "other" and without a user gesture. If we intercept them we end up
    // recursively loading the same page forever. Let WebKit handle them
    // normally.
    if (!is_user_gesture && navigation_type == WEBKIT_NAVIGATION_TYPE_OTHER) {
        return FALSE;
    }

    GUri* parsed_uri = g_uri_parse(uri, G_URI_FLAGS_NONE, nullptr);
    if (parsed_uri == nullptr) {
        return FALSE;
    }

    const gchar* scheme = g_uri_get_scheme(parsed_uri);
    std::string scheme_string = scheme != nullptr ? scheme : "";
    std::transform(scheme_string.begin(), scheme_string.end(), scheme_string.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (!scheme_string.empty() && scheme_string != "file") {
        g_message("GtkApp allowing external navigation. uri=%s scheme=%s", uri,
                  scheme_string.c_str());
        g_uri_unref(parsed_uri);
        return FALSE;
    }

    const gchar* path = g_uri_get_path(parsed_uri);
    const gchar* query = g_uri_get_query(parsed_uri);

    std::string path_string = path != nullptr ? path : "";
    std::string query_string = query != nullptr ? query : "";

    g_uri_unref(parsed_uri);

    std::string normalized_path = normalize_navigation_path(path_string);

    if (normalized_path.empty()) {
        return FALSE;
    }

    Language language = language_from_query(query_string, self->manager_.get_default_language());

    const bool navigating_to_menu =
        (normalized_path == "/" || normalized_path == "/index.html");
    const bool navigating_to_beaverphone = (normalized_path == "/apps/beaverphone");
    const bool navigating_to_beaveralarm = (normalized_path == "/apps/beaveralarm");
    const bool navigating_to_beaversystem = (normalized_path == "/apps/beaversystem");

    if (!navigating_to_menu && !navigating_to_beaverphone && !navigating_to_beaveralarm &&
        !navigating_to_beaversystem) {
        return FALSE;
    }

    self->manager_.set_default_language(language);

    bool handled_navigation = false;

    if (navigating_to_menu) {
        self->load_language(web_view, language);
        handled_navigation = true;
    } else if (navigating_to_beaverphone) {
#if defined(HAVE_GSTREAMER)
        self->set_camera_active(false);
#endif
        std::string html = self->manager_.beaverphone_page_html(
            language, BeaverphoneMenuLinkMode::kRelativeIndex);
        std::string base_uri = build_base_uri();
        if (html.empty()) {
            g_warning("GtkApp received empty BeaverPhone HTML for language: %s",
                      language_to_string(language));
        }
        webkit_web_view_load_html(web_view, html.c_str(), base_uri.c_str());
        handled_navigation = true;
    } else if (navigating_to_beaveralarm) {
#if defined(HAVE_GSTREAMER)
        self->set_camera_active(true);
#endif
        std::string html = self->manager_.beaveralarm_page_html(
            language, BeaverAlarmMenuLinkMode::kRelativeIndex);
        std::string base_uri = build_base_uri();
        if (html.empty()) {
            g_warning("GtkApp received empty BeaverAlarm HTML for language: %s",
                      language_to_string(language));
        }
        webkit_web_view_load_html(web_view, html.c_str(), base_uri.c_str());
        handled_navigation = true;
    } else if (navigating_to_beaversystem) {
#if defined(HAVE_GSTREAMER)
        self->set_camera_active(false);
#endif
        std::string html = self->manager_.beaversystem_page_html(
            language, BeaverSystemMenuLinkMode::kRelativeIndex);
        std::string base_uri = build_base_uri();
        if (html.empty()) {
            g_warning("GtkApp received empty BeaverSystem HTML for language: %s",
                      language_to_string(language));
        }
        webkit_web_view_load_html(web_view, html.c_str(), base_uri.c_str());
        handled_navigation = true;
    }

    if (!handled_navigation) {
        return FALSE;
    }

    webkit_policy_decision_ignore(decision);
    return TRUE;
}

gboolean GtkApp::on_console_message(WebKitWebView* /*web_view*/,
#if defined(WEBKIT_CONSOLE_MESSAGE_LEVEL_INFO)
                                    WebKitConsoleMessage* message,
#else
                                    gpointer message,
#endif
                                    gpointer /*user_data*/) {
#if !defined(WEBKIT_CONSOLE_MESSAGE_LEVEL_INFO)
    (void)message;
    return FALSE;
#else
    if (message == nullptr) {
        return FALSE;
    }

    const gchar* text = webkit_console_message_get_text(message);
    if (text == nullptr) {
        text = "";
    }

    const gchar* source_id = webkit_console_message_get_source_id(message);
    if (source_id == nullptr || source_id[0] == '\0') {
        source_id = "<unknown>";
    }

    const guint line = webkit_console_message_get_line(message);
    const WebKitConsoleMessageLevel level = webkit_console_message_get_level(message);
    const WebKitConsoleMessageSource source = webkit_console_message_get_source(message);

    const char* level_string = console_message_level_to_string(level);
    const char* source_string = console_message_source_to_string(source);

    gchar* formatted = g_strdup_printf("WebView console [%s/%s:%u] (%s) %s", source_string,
                                       source_id, line, level_string, text);

    switch (level) {
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_ERROR:
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_WARNING:
            g_warning("%s", formatted);
            break;
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_INFO:
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_LOG:
        case WEBKIT_CONSOLE_MESSAGE_LEVEL_DEBUG:
        default:
            g_message("%s", formatted);
            break;
    }

    g_free(formatted);
    return FALSE;
#endif
}

void GtkApp::load_language(WebKitWebView* web_view, Language language) {
#if defined(HAVE_GSTREAMER)
    set_camera_active(false);
#endif
    std::string html = manager_.to_html(language, MenuRouteMode::kKiosk);
    std::string base_uri = build_base_uri();
    if (html.empty()) {
        g_warning("GtkApp received empty menu HTML for language: %s",
                  language_to_string(language));
    }
    manager_.clear_navigation_history();
    remove_remote_navigation_controls(web_view);
    webkit_web_view_load_html(web_view, html.c_str(), base_uri.c_str());
}

void GtkApp::ensure_remote_navigation_controls(WebKitWebView* web_view) {
    if (web_view == nullptr) {
        return;
    }

    g_message("GtkApp injecting remote navigation controls.");
#if WEBKIT_CHECK_VERSION(2, 40, 0)
    webkit_web_view_evaluate_javascript(web_view, kRemoteBackButtonScript, -1, nullptr, nullptr,
                                        nullptr, nullptr, nullptr);
#else
    webkit_web_view_run_javascript(web_view, kRemoteBackButtonScript, nullptr, nullptr, nullptr);
#endif
}

void GtkApp::remove_remote_navigation_controls(WebKitWebView* web_view) {
    if (web_view == nullptr) {
        return;
    }

    g_message("GtkApp removing remote navigation controls.");
#if WEBKIT_CHECK_VERSION(2, 40, 0)
    webkit_web_view_evaluate_javascript(web_view, kRemoveRemoteBackButtonScript, -1, nullptr,
                                        nullptr, nullptr, nullptr, nullptr);
#else
    webkit_web_view_run_javascript(web_view, kRemoveRemoteBackButtonScript, nullptr, nullptr,
                                   nullptr);
#endif
}

void GtkApp::handle_remote_go_home() {
    if (web_view_ == nullptr) {
        g_warning("GtkApp received remote go-home request without an active web view.");
        return;
    }

    g_message("GtkApp received remote go-home request. Returning to kiosk menu.");
#if defined(HAVE_GSTREAMER)
    set_camera_active(false);
#endif
    load_language(web_view_, manager_.get_default_language());
}

void GtkApp::on_load_changed(WebKitWebView* web_view, WebKitLoadEvent load_event,
                             gpointer user_data) {
    auto* self = static_cast<GtkApp*>(user_data);
    if (self == nullptr || web_view == nullptr) {
        return;
    }

    if (load_event != WEBKIT_LOAD_FINISHED) {
        return;
    }

    const gchar* uri = webkit_web_view_get_uri(web_view);
    std::string uri_string = uri != nullptr ? uri : "";
    if (uri_string.empty()) {
        self->remove_remote_navigation_controls(web_view);
        return;
    }

    std::optional<RouteMatch> matched_route =
        self->manager_.match_route_for_uri(uri_string, MenuRouteMode::kKiosk);
    if (!matched_route.has_value()) {
        self->remove_remote_navigation_controls(web_view);
        return;
    }

    const std::string app_name = matched_route->app != nullptr ? matched_route->app->name : uri_string;
    self->manager_.record_navigation(app_name, MenuRouteMode::kKiosk);

    if (matched_route->route != nullptr && matched_route->route->remote) {
        self->ensure_remote_navigation_controls(web_view);
    } else {
        self->remove_remote_navigation_controls(web_view);
    }
}

void GtkApp::on_script_message_received(WebKitUserContentManager* /*content_manager*/,
                                        WebKitJavascriptResult* result, gpointer user_data) {
    auto* self = static_cast<GtkApp*>(user_data);
    if (self == nullptr || result == nullptr) {
        return;
    }

    const auto message = [&result]() -> std::optional<std::string> {
        if (result == nullptr) {
            return std::nullopt;
        }

#if WEBKIT_CHECK_VERSION(2, 40, 0)
        JSGlobalContextRef context = webkit_javascript_result_get_global_context(result);
        JSValueRef value_ref = webkit_javascript_result_get_value(result);
        if (context == nullptr || value_ref == nullptr) {
            return std::nullopt;
        }

        JSStringRef js_string = JSValueToStringCopy(context, value_ref, nullptr);
        if (js_string == nullptr) {
            return std::nullopt;
        }

        const size_t maximum_size = JSStringGetMaximumUTF8CStringSize(js_string);
        std::string buffer;
        buffer.resize(maximum_size);
        const size_t written = JSStringGetUTF8CString(js_string, buffer.data(), maximum_size);
        JSStringRelease(js_string);

        if (written == 0) {
            return std::nullopt;
        }

        buffer.resize(written - 1);  // Exclude trailing null terminator.
        return buffer;
#else
        const auto g_free_deleter = [](gchar* ptr) {
            if (ptr != nullptr) {
                g_free(ptr);
            }
        };

        using GCharPtr = std::unique_ptr<gchar, decltype(g_free_deleter)>;

        GCharPtr value(webkit_javascript_result_to_string(result), g_free_deleter);
        if (!value) {
            return std::nullopt;
        }
        return std::string(value.get());
#endif
    }();

    if (!message.has_value()) {
        return;
    }

    if (*message == "go-home") {
        self->handle_remote_go_home();
    }
}

void GtkApp::set_camera_active(bool active) {
#if defined(HAVE_GSTREAMER)
    camera_requested_active_ = active;
    if (active) {
        start_camera_pipeline();
    } else {
        stop_camera_pipeline();
    }
#else
    (void)active;
#endif
}

#if defined(HAVE_GSTREAMER)

namespace {
constexpr const int kCameraOverlayMargin = 24;
constexpr const int kCameraOverlaySpacing = 8;
constexpr const int kCameraMinimumWidth = 640;
constexpr const int kCameraMinimumHeight = 360;
}  // namespace

void GtkApp::ensure_gstreamer_initialized() {
    if (gstreamer_initialized_) {
        return;
    }
    gst_init(nullptr, nullptr);
    gstreamer_initialized_ = true;
}

void GtkApp::ensure_camera_overlay() {
    if (overlay_root_ == nullptr) {
        return;
    }

    if (camera_overlay_ != nullptr && !GTK_IS_WIDGET(camera_overlay_)) {
        g_warning("GtkApp detected invalid CCTV overlay widget. Resetting overlay state.");
        camera_overlay_ = nullptr;
        camera_frame_ = nullptr;
        camera_status_label_ = nullptr;
        camera_video_widget_ = nullptr;
    }

    if (camera_overlay_ != nullptr) {
        return;
    }

    GtkWidget* container = gtk_box_new(GTK_ORIENTATION_VERTICAL, kCameraOverlaySpacing);
    camera_overlay_ = container;
    gtk_widget_set_hexpand(container, FALSE);
    gtk_widget_set_vexpand(container, FALSE);
    gtk_widget_set_halign(container, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(container, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(container, kCameraOverlayMargin);
    gtk_widget_set_margin_end(container, kCameraOverlayMargin);
    gtk_label_set_wrap_mode(GTK_LABEL(camera_status_label_), PANGO_WRAP_WORD_CHAR);
    gtk_widget_set_margin_bottom(container, kCameraOverlayMargin);
    gtk_widget_set_size_request(container, kCameraMinimumWidth, kCameraMinimumHeight);

    GtkWidget* frame = gtk_frame_new(nullptr);
    camera_frame_ = frame;
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
#if GTK_MAJOR_VERSION >= 4
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay_root_), frame);
    gtk_frame_set_child(GTK_FRAME(frame), container);
#else
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay_root_), frame);
    gtk_container_add(GTK_CONTAINER(frame), container);
#endif

    camera_status_label_ = gtk_label_new("Connecting to CCTV feed…");
    gtk_label_set_xalign(GTK_LABEL(camera_status_label_), 0.0f);
#if GTK_CHECK_VERSION(4, 10, 0)
    gtk_label_set_wrap(GTK_LABEL(camera_status_label_), TRUE);
#else
    gtk_label_set_line_wrap(GTK_LABEL(camera_status_label_), TRUE);
#endif
#if GTK_MAJOR_VERSION >= 4
    gtk_box_append(GTK_BOX(container), camera_status_label_);
#else
    gtk_box_pack_start(GTK_BOX(container), camera_status_label_, FALSE, FALSE, 0);
#endif
    set_widget_visible(camera_status_label_, true);
    set_widget_visible(frame, false);
}

void GtkApp::set_camera_status(const std::string& message) {
    if (camera_status_label_ == nullptr) {
        return;
    }

    if (!GTK_IS_LABEL(camera_status_label_)) {
        g_warning("GtkApp camera status label pointer is invalid. Disabling status updates.");
        camera_status_label_ = nullptr;
        return;
    }
    if (message.empty()) {
        gtk_label_set_text(GTK_LABEL(camera_status_label_), "");
        set_widget_visible(camera_status_label_, false);
        return;
    }
    gtk_label_set_text(GTK_LABEL(camera_status_label_), message.c_str());
    set_widget_visible(camera_status_label_, true);
}

GstElement* GtkApp::create_camera_sink() {
    const char* const candidates[] = {"gtksink", "gtkglsink", "autovideosink", nullptr};
    for (const char* const* name = candidates; *name != nullptr; ++name) {
        GstElement* sink = gst_element_factory_make(*name, "beaver-camera-sink");
        if (sink != nullptr) {
            g_message("GtkApp using %s for CCTV pipeline video sink.", *name);
            return sink;
        }
    }
    g_warning("GtkApp could not create a GTK-compatible video sink. CCTV feed disabled.");
    return nullptr;
}

void GtkApp::attach_camera_widget_from_sink(GstElement* sink) {
    if (sink == nullptr || camera_overlay_ == nullptr) {
        return;
    }

    if (camera_video_widget_ != nullptr) {
        return;
    }

    if (!g_object_class_find_property(G_OBJECT_GET_CLASS(sink), "widget")) {
        return;
    }

    GtkWidget* widget = nullptr;
    g_object_get(sink, "widget", &widget, nullptr);
    if (widget == nullptr) {
        return;
    }

    gtk_widget_set_hexpand(widget, TRUE);
    gtk_widget_set_vexpand(widget, TRUE);
#if GTK_MAJOR_VERSION >= 4
    gtk_box_append(GTK_BOX(camera_overlay_), widget);
#else
    gtk_box_pack_start(GTK_BOX(camera_overlay_), widget, TRUE, TRUE, 0);
#endif
    camera_video_widget_ = widget;
    set_widget_visible(camera_status_label_, true);
}

void GtkApp::attach_camera_bus() {
    if (camera_pipeline_ == nullptr) {
        return;
    }
    GstBus* bus = gst_element_get_bus(camera_pipeline_);
    if (bus == nullptr) {
        return;
    }
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error", G_CALLBACK(GtkApp::on_camera_bus_error), this);
    g_signal_connect(bus, "message::eos", G_CALLBACK(GtkApp::on_camera_bus_eos), this);
    g_signal_connect(bus, "message::state-changed", G_CALLBACK(GtkApp::on_camera_bus_state_changed), this);
    gst_object_unref(bus);
}

void GtkApp::detach_camera_bus() {
    if (camera_pipeline_ == nullptr) {
        return;
    }
    GstBus* bus = gst_element_get_bus(camera_pipeline_);
    if (bus == nullptr) {
        return;
    }
    g_signal_handlers_disconnect_by_data(bus, this);
    gst_bus_remove_signal_watch(bus);
    gst_object_unref(bus);
}

void GtkApp::start_camera_pipeline() {
    if (!camera_requested_active_) {
        return;
    }
    ensure_gstreamer_initialized();
    ensure_camera_overlay();

    if (camera_overlay_ == nullptr) {
        g_warning("GtkApp cannot display CCTV feed: overlay not available.");
        return;
    }

    if (camera_pipeline_ != nullptr) {
        if (camera_frame_ != nullptr) {
            set_widget_visible(camera_frame_, true);
        }
        set_widget_visible(camera_overlay_, true);
        return;
    }

    CctvConfig config = load_cctv_config_from_env();
    const std::string uri = config.rtsp_uri(true);
    if (uri.empty()) {
        g_warning("GtkApp CCTV configuration missing RTSP URI. Unable to start stream.");
        set_camera_status("Camera feed unavailable.");
        if (camera_frame_ != nullptr) {
            set_widget_visible(camera_frame_, true);
        }
        set_widget_visible(camera_overlay_, true);
        return;
    }

    set_camera_status("Connecting to CCTV feed…");

    GstElement* pipeline = gst_element_factory_make("playbin", "beaver-camera-playbin");
    if (pipeline == nullptr) {
        g_warning("GtkApp failed to create playbin pipeline for CCTV feed.");
        set_camera_status("Unable to create CCTV pipeline.");
        if (camera_frame_ != nullptr) {
            set_widget_visible(camera_frame_, true);
        }
        set_widget_visible(camera_overlay_, true);
        return;
    }

    GstElement* sink = create_camera_sink();
    if (sink == nullptr) {
        gst_object_unref(pipeline);
        set_camera_status("Missing GTK video sink for CCTV feed.");
        if (camera_frame_ != nullptr) {
            set_widget_visible(camera_frame_, true);
        }
        set_widget_visible(camera_overlay_, true);
        return;
    }

    attach_camera_widget_from_sink(sink);
    g_object_set(pipeline, "video-sink", sink, nullptr);
    gst_object_unref(sink);

    g_object_set(pipeline, "uri", uri.c_str(), nullptr);

    camera_pipeline_ = pipeline;
    attach_camera_bus();

    if (camera_frame_ != nullptr) {
        set_widget_visible(camera_frame_, true);
    }
    set_widget_visible(camera_overlay_, true);

    const GstStateChangeReturn state = gst_element_set_state(camera_pipeline_, GST_STATE_PLAYING);
    if (state == GST_STATE_CHANGE_FAILURE) {
        g_warning("GtkApp could not start CCTV pipeline. State change failed.");
        detach_camera_bus();
        gst_object_unref(camera_pipeline_);
        camera_pipeline_ = nullptr;
        set_camera_status("Unable to start CCTV feed.");
        if (camera_frame_ != nullptr) {
            set_widget_visible(camera_frame_, true);
        }
        set_widget_visible(camera_overlay_, true);
        if (camera_video_widget_ != nullptr) {
#if GTK_MAJOR_VERSION >= 4
            gtk_box_remove(GTK_BOX(camera_overlay_), camera_video_widget_);
#else
            gtk_container_remove(GTK_CONTAINER(camera_overlay_), camera_video_widget_);
#endif
            camera_video_widget_ = nullptr;
        }
    }
}

void GtkApp::stop_camera_pipeline() {
    if (camera_pipeline_ != nullptr) {
        detach_camera_bus();
        gst_element_set_state(camera_pipeline_, GST_STATE_NULL);
        gst_object_unref(camera_pipeline_);
        camera_pipeline_ = nullptr;
    }

    if (camera_video_widget_ != nullptr) {
#if GTK_MAJOR_VERSION >= 4
        gtk_box_remove(GTK_BOX(camera_overlay_), camera_video_widget_);
#else
        gtk_container_remove(GTK_CONTAINER(camera_overlay_), camera_video_widget_);
#endif
        camera_video_widget_ = nullptr;
    }

    if (!camera_requested_active_) {
        set_camera_status("");
        if (camera_overlay_ != nullptr) {
            set_widget_visible(camera_overlay_, false);
        }
        if (camera_frame_ != nullptr) {
            set_widget_visible(camera_frame_, false);
        }
    }
}

void GtkApp::on_application_shutdown(GApplication* /*application*/, gpointer user_data) {
    auto* self = static_cast<GtkApp*>(user_data);
    if (self == nullptr) {
        return;
    }
    self->camera_requested_active_ = false;
    self->stop_camera_pipeline();
}

void GtkApp::on_camera_bus_error(GstBus* /*bus*/, GstMessage* message, gpointer user_data) {
    auto* self = static_cast<GtkApp*>(user_data);
    if (self == nullptr || message == nullptr) {
        return;
    }

    GError* error = nullptr;
    gchar* debug = nullptr;
    gst_message_parse_error(message, &error, &debug);

    const char* error_message = (error != nullptr && error->message != nullptr) ? error->message : "unknown error";
    g_warning("GtkApp CCTV pipeline error: %s", error_message);
    if (debug != nullptr && debug[0] != '\0') {
        g_message("GtkApp CCTV pipeline debug: %s", debug);
    }

    if (error != nullptr) {
        g_error_free(error);
    }
    if (debug != nullptr) {
        g_free(debug);
    }

    self->set_camera_status("Camera feed error. Reconnecting…");
    self->stop_camera_pipeline();

    if (self->camera_requested_active_) {
        g_timeout_add_seconds(2, [](gpointer data) -> gboolean {
            auto* app = static_cast<GtkApp*>(data);
            if (app != nullptr && app->camera_requested_active_) {
                app->start_camera_pipeline();
            }
            return G_SOURCE_REMOVE;
        }, self);
    }
}

void GtkApp::on_camera_bus_eos(GstBus* /*bus*/, GstMessage* /*message*/, gpointer user_data) {
    auto* self = static_cast<GtkApp*>(user_data);
    if (self == nullptr) {
        return;
    }
    g_message("GtkApp CCTV pipeline received EOS. Restarting stream.");
    self->set_camera_status("Camera feed ended. Reconnecting…");
    self->stop_camera_pipeline();
    if (self->camera_requested_active_) {
        self->start_camera_pipeline();
    }
}

void GtkApp::on_camera_bus_state_changed(GstBus* /*bus*/, GstMessage* message, gpointer user_data) {
    auto* self = static_cast<GtkApp*>(user_data);
    if (self == nullptr || message == nullptr) {
        return;
    }

    if (GST_MESSAGE_SRC(message) != GST_OBJECT(self->camera_pipeline_)) {
        return;
    }

    GstState old_state = GST_STATE_NULL;
    GstState new_state = GST_STATE_NULL;
    GstState pending_state = GST_STATE_VOID_PENDING;
    gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);

    switch (new_state) {
        case GST_STATE_READY:
        case GST_STATE_PAUSED:
            self->set_camera_status("Connecting to CCTV feed…");
            break;
        case GST_STATE_PLAYING:
            self->set_camera_status("");
            break;
        default:
            break;
    }
}

#endif  // defined(HAVE_GSTREAMER)
