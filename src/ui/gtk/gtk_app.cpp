#include "ui/gtk/gtk_app.h"

#include <filesystem>
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
#if GTK_MAJOR_VERSION >= 4
    gtk_window_set_child(GTK_WINDOW(window), webview);
#else
    gtk_container_add(GTK_CONTAINER(window), webview);
#endif

    std::string html = manager_.to_html();
    std::string base_uri = build_base_uri();
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(webview), html.c_str(), base_uri.c_str());

#if GTK_MAJOR_VERSION >= 4
    gtk_window_present(GTK_WINDOW(window));
#else
    gtk_widget_show_all(window);
#endif
}
