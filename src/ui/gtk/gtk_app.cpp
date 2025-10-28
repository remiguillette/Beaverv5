#include "ui/gtk/gtk_app.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>

#include <webkit2/webkit2.h>
#include <glib.h>

GtkApp::GtkApp(AppManager& manager) : manager_(manager) {}

int GtkApp::run(int argc, char** argv) {
    GtkApplication* application = gtk_application_new("com.beaver.kiosk", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(application, "activate", G_CALLBACK(GtkApp::on_activate), this);

    int status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}

void GtkApp::on_activate(GtkApplication* application, gpointer user_data) {
    auto* self = static_cast<GtkApp*>(user_data);
    self->build_ui(application);
}

namespace {
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
}  // namespace

void GtkApp::build_ui(GtkApplication* application) {
    GtkWidget* window = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(window), "BeaverKiosk");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

    GtkWidget* webview = webkit_web_view_new();
    g_signal_connect(webview, "decide-policy", G_CALLBACK(GtkApp::on_decide_policy), this);
#if GTK_MAJOR_VERSION >= 4
    gtk_window_set_child(GTK_WINDOW(window), webview);
#else
    gtk_container_add(GTK_CONTAINER(window), webview);
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
    const guint mouse_button = webkit_navigation_action_get_mouse_button(action);
    const guint modifiers = webkit_navigation_action_get_modifiers(action);

    g_message("GtkApp navigation action received:");
    g_message("    \342\206\222 URI: %s", uri);
    g_message("    \342\206\222 Type: %s (%d)", navigation_type_to_string(navigation_type),
              static_cast<int>(navigation_type));
    g_message("    \342\206\222 User gesture: %s", is_user_gesture ? "true" : "false");
    g_message("    \342\206\222 Mouse button: %u", mouse_button);
    g_message("    \342\206\222 Modifiers: 0x%x", modifiers);

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

    if (!navigating_to_menu && !navigating_to_beaverphone) {
        return FALSE;
    }

    self->manager_.set_default_language(language);

    if (navigating_to_menu) {
        self->load_language(web_view, language);
    } else if (navigating_to_beaverphone) {
        std::string html = self->manager_.beaverphone_page_html(
            language, BeaverphoneMenuLinkMode::kRelativeIndex);
        std::string base_uri = build_base_uri();
        if (html.empty()) {
            g_warning("GtkApp received empty BeaverPhone HTML for language: %s",
                      language_to_string(language));
        }
        webkit_web_view_load_html(web_view, html.c_str(), base_uri.c_str());
    }

    webkit_policy_decision_ignore(decision);
    return TRUE;
}

void GtkApp::load_language(WebKitWebView* web_view, Language language) {
    std::string html = manager_.to_html(language, MenuRouteMode::kKiosk);
    std::string base_uri = build_base_uri();
    if (html.empty()) {
        g_warning("GtkApp received empty menu HTML for language: %s",
                  language_to_string(language));
    }
    webkit_web_view_load_html(web_view, html.c_str(), base_uri.c_str());
}
