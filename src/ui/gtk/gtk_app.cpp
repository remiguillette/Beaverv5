#include "ui/gtk/gtk_app.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include <webkit2/webkit2.h>

#include "core/resource_paths.h"

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
std::filesystem::path resolve_public_directory() {
    namespace fs = std::filesystem;
    fs::path public_dir = resource_paths::public_directory();
    std::error_code ec;
    fs::path canonical = fs::weakly_canonical(public_dir, ec);
    return ec ? public_dir : canonical;
}

std::string build_base_uri(const std::filesystem::path& public_dir) {
    return resource_paths::file_uri_from_path(public_dir);
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

    base_path_ = resolve_public_directory();
    base_uri_ = build_base_uri(base_path_);
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
    std::string path;
    std::string query;

    const std::size_t scheme_position = uri_string.find("://");
    if (scheme_position != std::string::npos) {
        const std::size_t path_start = uri_string.find('/', scheme_position + 3);
        if (path_start != std::string::npos) {
            path = uri_string.substr(path_start);
        }
    } else if (!uri_string.empty() && uri_string.front() == '/') {
        path = uri_string;
    }

    path = self->normalize_path(path);

    const std::size_t query_position = path.find('?');
    if (query_position != std::string::npos) {
        query = path.substr(query_position + 1);
        path = path.substr(0, query_position);
    }

    if (path == "/index.html") {
        path = "/";
    }

    std::stringstream query_stream(query);
    std::string parameter;

    Language language = self->manager_.get_default_language();
    bool language_changed = false;

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
            if (value == "en") {
                language = Language::English;
                language_changed = true;
            } else if (value == "fr") {
                language = Language::French;
                language_changed = true;
            }
        }
    }

    bool handled = false;
    if (path == "/" || path.empty()) {
        if (language_changed) {
            self->manager_.set_default_language(language);
        }
        self->load_language(web_view, language);
        handled = true;
    } else if (path == "/apps/beaverphone") {
        if (language_changed) {
            self->manager_.set_default_language(language);
        }
        self->load_beaverphone(web_view, language);
        handled = true;
    }

    if (handled) {
        webkit_policy_decision_ignore(decision);
        return TRUE;
    }

    return FALSE;
}

void GtkApp::load_language(WebKitWebView* web_view, Language language) {
    load_html(web_view, manager_.to_html(language));
}

void GtkApp::load_beaverphone(WebKitWebView* web_view, Language language) {
    load_html(web_view, manager_.beaverphone_page_html(language));
}

void GtkApp::load_html(WebKitWebView* web_view, std::string html) {
    if (base_uri_.empty()) {
        base_path_ = resolve_public_directory();
        base_uri_ = build_base_uri(base_path_);
    }
    current_document_html_ = std::move(html);
    webkit_web_view_load_html(web_view, current_document_html_.c_str(), base_uri_.c_str());
}

std::string GtkApp::normalize_path(const std::string& raw_path) const {
    if (raw_path.empty()) {
        return "/";
    }

    if (raw_path == "/") {
        return raw_path;
    }

    if (!base_path_.empty()) {
        const std::string base = base_path_.generic_string();
        if (!base.empty()) {
            const std::string base_with_slash = base + "/";
            if (raw_path.rfind(base_with_slash, 0) == 0) {
                std::string remainder = raw_path.substr(base_with_slash.size());
                if (remainder.empty()) {
                    return "/";
                }
                if (remainder.front() != '/') {
                    remainder.insert(remainder.begin(), '/');
                }
                return remainder;
            }
            if (raw_path == base) {
                return "/";
            }
        }
    }

    return raw_path;
}
