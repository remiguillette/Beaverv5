#pragma once

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <filesystem>
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
    void build_ui(GtkApplication* application);
    void load_language(WebKitWebView* web_view, Language language);
    void load_beaverphone(WebKitWebView* web_view, Language language);
    void load_html(WebKitWebView* web_view, const std::string& html);
    std::string normalize_path(const std::string& raw_path) const;

    AppManager& manager_;
    std::string base_uri_;
    std::filesystem::path base_path_;
};
