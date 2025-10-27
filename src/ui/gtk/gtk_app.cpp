#include "ui/gtk/gtk_app.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>

#include <webkit2/webkit2.h>

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
    std::string uri = "file://" + public_dir.string();
    if (!uri.empty() && uri.back() != '/') {
        uri.push_back('/');
    }
    return uri;
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

    load_language(WEBKIT_WEB_VIEW(webview), manager_.get_default_language());

#if GTK_MAJOR_VERSION >= 4
    gtk_window_present(GTK_WINDOW(window));
#else
    gtk_widget_show_all(window);
#endif
}

gboolean GtkApp::on_decide_policy(WebKitWebView* web_view, WebKitPolicyDecision* decision,
                                  WebKitPolicyDecisionType decision_type, gpointer user_data) {
    if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
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

    std::string uri_string(uri);
    const std::size_t query_position = uri_string.find('?');
    if (query_position == std::string::npos) {
        return FALSE;
    }

    std::string query = uri_string.substr(query_position + 1);
    std::stringstream query_stream(query);
    std::string parameter;
    std::string language_value;

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
        if (key == "lang") {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            language_value = std::move(value);
            break;
        }
    }

    if (language_value.empty()) {
        return FALSE;
    }

    Language language = self->manager_.get_default_language();
    bool handled = false;
    if (language_value == "en") {
        language = Language::English;
        handled = true;
    } else if (language_value == "fr") {
        language = Language::French;
        handled = true;
    }

    if (!handled) {
        return FALSE;
    }

    self->manager_.set_default_language(language);
    self->load_language(web_view, language);
    webkit_policy_decision_ignore(decision);
    return TRUE;
}

void GtkApp::load_language(WebKitWebView* web_view, Language language) {
    std::string html = manager_.to_html(language);
    std::string base_uri = build_base_uri();
    webkit_web_view_load_html(web_view, html.c_str(), base_uri.c_str());
}
