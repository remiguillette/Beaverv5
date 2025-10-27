#pragma once

#include <gtk/gtk.h>

#include "core/app_manager.h"

class GtkApp {
public:
    explicit GtkApp(AppManager& manager);
    int run(int argc, char** argv);

private:
    static void on_activate(GtkApplication* application, gpointer user_data);
    void build_ui(GtkApplication* application);
    void apply_css(GtkWidget* root);
    GtkWidget* create_app_tile(const AppTile& app) const;

    AppManager& manager_;
};
