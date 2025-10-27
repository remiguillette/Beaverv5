#include "ui/gtk/gtk_app.h"

#include <algorithm>
#include <cctype>
#include <string>

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

void GtkApp::apply_css(GtkWidget* root) {
    static const char* css = R"(
        .app-window {
            background-image: linear-gradient(135deg, #050505, #161b3d);
            color: #f8fafc;
            font-family: 'Inter', system-ui, sans-serif;
        }

        .menu-root {
            margin: 32px;
            padding: 16px;
        }

        .menu-header {
            margin-bottom: 24px;
        }

        .menu-title {
            font-size: 42px;
            font-weight: 700;
            margin-bottom: 4px;
        }

        .menu-subtitle {
            font-size: 16px;
            opacity: 0.8;
        }

        .menu-grid {
            padding: 8px;
        }

        flowboxchild {
            padding: 8px;
        }

        .app-tile {
            border-radius: 20px;
            padding: 28px 24px;
            background: rgba(15, 23, 42, 0.6);
            border: none;
            transition: transform 120ms ease, box-shadow 120ms ease;
            color: inherit;
        }

        .app-tile:hover {
            transform: translateY(-4px);
            box-shadow: 0 16px 32px rgba(15, 23, 42, 0.35);
        }

        .app-tile-box {
            align-items: flex-start;
        }

        .app-tile-icon {
            font-size: 48px;
            font-weight: 300;
            margin-bottom: 12px;
        }

        .app-tile-name {
            font-size: 18px;
            font-weight: 500;
        }

        .menu-footer {
            margin-top: 24px;
            font-size: 12px;
            opacity: 0.6;
        }

        .accent-violet {
            background-image: linear-gradient(135deg, rgba(139, 92, 246, 0.25), rgba(76, 29, 149, 0.25));
        }

        .accent-cyan {
            background-image: linear-gradient(135deg, rgba(34, 211, 238, 0.25), rgba(14, 116, 144, 0.25));
        }

        .accent-amber {
            background-image: linear-gradient(135deg, rgba(251, 191, 36, 0.25), rgba(217, 119, 6, 0.25));
        }

        .accent-red {
            background-image: linear-gradient(135deg, rgba(248, 113, 113, 0.25), rgba(185, 28, 28, 0.25));
        }

        .accent-green {
            background-image: linear-gradient(135deg, rgba(74, 222, 128, 0.25), rgba(22, 101, 52, 0.25));
        }
    )";

    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1);

    GdkDisplay* display = gtk_widget_get_display(root);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

GtkWidget* GtkApp::create_app_tile(const AppTile& app) const {
    GtkWidget* button = gtk_button_new();
    gtk_widget_add_css_class(button, "app-tile");
    std::string accent_class = "accent-" + app.accent;
    gtk_widget_add_css_class(button, accent_class.c_str());
    gtk_button_set_has_frame(GTK_BUTTON(button), FALSE);
    gtk_widget_set_hexpand(button, TRUE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_add_css_class(box, "app-tile-box");

    GtkWidget* icon_label = gtk_label_new(nullptr);
    char initial = !app.name.empty() ? static_cast<char>(std::toupper(app.name.front())) : 'A';
    std::string icon_text(1, initial);
    gtk_label_set_text(GTK_LABEL(icon_label), icon_text.c_str());
    gtk_widget_add_css_class(icon_label, "app-tile-icon");

    GtkWidget* name_label = gtk_label_new(app.name.c_str());
    gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);
    gtk_widget_add_css_class(name_label, "app-tile-name");

    gtk_box_append(GTK_BOX(box), icon_label);
    gtk_box_append(GTK_BOX(box), name_label);

    gtk_button_set_child(GTK_BUTTON(button), box);
    return button;
}

void GtkApp::build_ui(GtkApplication* application) {
    GtkWidget* window = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(window), "BeaverKiosk");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_widget_add_css_class(window, "app-window");

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_add_css_class(root, "menu-root");
    gtk_widget_set_margin_top(root, 32);
    gtk_widget_set_margin_bottom(root, 32);
    gtk_widget_set_margin_start(root, 48);
    gtk_widget_set_margin_end(root, 48);

    apply_css(window);
    gtk_window_set_child(GTK_WINDOW(window), root);

    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(header, "menu-header");

    GtkWidget* title = gtk_label_new("BeaverKiosk");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_widget_add_css_class(title, "menu-title");

    GtkWidget* subtitle = gtk_label_new("Unified application launcher • Native GTK 4 edition");
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0);
    gtk_widget_add_css_class(subtitle, "menu-subtitle");

    gtk_box_append(GTK_BOX(header), title);
    gtk_box_append(GTK_BOX(header), subtitle);

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget* flowbox = gtk_flow_box_new();
    gtk_widget_add_css_class(flowbox, "menu-grid");
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flowbox), 3);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), 24);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), 24);

    const auto& apps = manager_.get_available_apps();
    for (const auto& app : apps) {
        GtkWidget* tile = create_app_tile(app);
        gtk_flow_box_append(GTK_FLOW_BOX(flowbox), tile);
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), flowbox);

    GtkWidget* footer = gtk_label_new("C++ • Native Interfaces • Beaver Suite ©2025");
    gtk_label_set_xalign(GTK_LABEL(footer), 0.0);
    gtk_widget_add_css_class(footer, "menu-footer");

    gtk_box_append(GTK_BOX(root), header);
    gtk_box_append(GTK_BOX(root), scrolled);
    gtk_box_append(GTK_BOX(root), footer);

    gtk_window_present(GTK_WINDOW(window));
}
