#pragma once

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <string>

#include "core/app_manager.h"

class GtkApp {
public:
    explicit GtkApp(AppManager& manager);
    int run(int argc, char** argv);

private:
    static void on_activate(GtkApplication* application, gpointer user_data);
    static gboolean on_decide_policy(WebKitWebView* web_view, WebKitPolicyDecision* decision,
                                     WebKitPolicyDecisionType decision_type, gpointer user_data);
    static gboolean on_permission_request(WebKitWebView* web_view,
                                          WebKitPermissionRequest* request,
                                          gpointer user_data);
    static gboolean on_console_message(WebKitWebView* web_view,
#if defined(WEBKIT_CONSOLE_MESSAGE_LEVEL_INFO)
                                       WebKitConsoleMessage* message,
#else
                                       gpointer message,
#endif
                                       gpointer user_data);
    static void on_load_changed(WebKitWebView* web_view, WebKitLoadEvent load_event,
                                gpointer user_data);
    static void on_script_message_received(WebKitUserContentManager* content_manager,
                                           WebKitJavascriptResult* result, gpointer user_data);

    void build_ui(GtkApplication* application);
    void load_language(WebKitWebView* web_view, Language language);
    void load_beaveralarm_page(WebKitWebView* web_view, Language language);
    void load_beavertask_page(WebKitWebView* web_view, Language language);
    void ensure_remote_navigation_controls(WebKitWebView* web_view);
    void remove_remote_navigation_controls(WebKitWebView* web_view);
    void handle_remote_go_home();
    void prompt_media_permission(WebKitPermissionRequest* request);
    GtkWindow* resolve_parent_window() const;

    AppManager& manager_;
    WebKitWebView* web_view_ = nullptr;
    GtkWidget* overlay_root_ = nullptr;
};
