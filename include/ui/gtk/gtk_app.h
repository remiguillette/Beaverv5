#pragma once

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

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

    void suppress_next_decide_policy();

    AppManager& manager_;
    bool suppress_decide_policy_request_ = false;
};
