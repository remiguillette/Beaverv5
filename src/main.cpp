#include <gtk/gtk.h>

#include <array>
#include <string>
#include <string_view>

struct AppTile {
    std::string_view name;
    std::string_view description;
    std::string_view accent_class;
};

struct AppState {
    GtkLabel* status_label = nullptr;
};

static const std::array<AppTile, 6> kApps = {{{
    "Code Editor",
    "Lightweight editor for quick edits and configuration tweaks.",
    "accent-violet"
}, {
    "Terminal",
    "Open a secure local shell for administrative tasks.",
    "accent-blue"
}, {
    "File Browser",
    "Inspect and organize local project files.",
    "accent-green"
}, {
    "Task Monitor",
    "Review CPU, memory, and I/O usage in real time.",
    "accent-amber"
}, {
    "Documentation",
    "Read locally cached manuals and API references.",
    "accent-rose"
}, {
    "Settings",
    "Configure kiosk preferences and accessibility options.",
    "accent-slate"
}}};

static void on_tile_clicked(GtkButton* /*button*/, gpointer user_data) {
    AppState* state = static_cast<AppState*>(g_object_get_data(G_OBJECT(user_data), "app-state"));
    const AppTile* tile = static_cast<const AppTile*>(g_object_get_data(G_OBJECT(user_data), "app-tile"));

    if (!state || !state->status_label || !tile) {
        return;
    }

    std::string message = std::string("Preparing to launch ") + std::string(tile->name) + " locally...";
    gtk_label_set_text(state->status_label, message.c_str());
}

static GtkWidget* build_tile_widget(const AppTile& tile, AppState* state) {
    GtkWidget* button = gtk_button_new();
    GtkStyleContext* button_context = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(button_context, "app-tile");
    gtk_style_context_add_class(button_context, tile.accent_class.data());

    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(content_box, 8);
    gtk_widget_set_margin_bottom(content_box, 8);
    gtk_widget_set_margin_start(content_box, 8);
    gtk_widget_set_margin_end(content_box, 8);

    GtkWidget* name_label = gtk_label_new(tile.name.data());
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(name_label), 0.0f);
    gtk_style_context_add_class(gtk_widget_get_style_context(name_label), "app-title");

    GtkWidget* description_label = gtk_label_new(tile.description.data());
    gtk_widget_set_halign(description_label, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(description_label), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(description_label), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(description_label), "app-description");

    gtk_box_append(GTK_BOX(content_box), name_label);
    gtk_box_append(GTK_BOX(content_box), description_label);

    gtk_button_set_child(GTK_BUTTON(button), content_box);

    g_object_set_data(G_OBJECT(button), "app-state", state);
    g_object_set_data(G_OBJECT(button), "app-tile", const_cast<AppTile*>(&tile));
    g_signal_connect(button, "clicked", G_CALLBACK(on_tile_clicked), button);

    return button;
}

static void load_css() {
    static constexpr char kCss[] =
        ".app-tile {"
        "  border-radius: 16px;"
        "  padding: 12px;"
        "  background: #1f2933;"
        "  color: #f8fafc;"
        "  min-width: 220px;"
        "  min-height: 120px;"
        "  text-align: left;"
        "  box-shadow: 0 12px 30px rgba(15, 23, 42, 0.35);"
        "  transition: transform 120ms ease, box-shadow 120ms ease;"
        "}"
        ".app-tile:hover {"
        "  filter: brightness(1.08);"
        "  transform: translateY(-4px);"
        "  box-shadow: 0 16px 40px rgba(15, 23, 42, 0.45);"
        "}"
        ".app-title {"
        "  font-size: 18px;"
        "  font-weight: 600;"
        "  letter-spacing: 0.02em;"
        "}"
        ".app-description {"
        "  opacity: 0.85;"
        "  font-size: 13px;"
        "  line-height: 1.35;"
        "}"
        ".status-frame {"
        "  border-radius: 12px;"
        "  border: 1px solid rgba(148, 163, 184, 0.35);"
        "  background: rgba(15, 23, 42, 0.7);"
        "}"
        ".heading-title {"
        "  font-size: 28px;"
        "  font-weight: 700;"
        "  letter-spacing: 0.01em;"
        "  margin-bottom: 4px;"
        "}"
        ".accent-violet { background: linear-gradient(135deg, #7c3aed, #5b21b6); }"
        ".accent-blue { background: linear-gradient(135deg, #2563eb, #1d4ed8); }"
        ".accent-green { background: linear-gradient(135deg, #16a34a, #047857); }"
        ".accent-amber { background: linear-gradient(135deg, #f59e0b, #d97706); }"
        ".accent-rose { background: linear-gradient(135deg, #f43f5e, #be123c); }"
        ".accent-slate { background: linear-gradient(135deg, #475569, #1e293b); }";

    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, kCss, -1);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static void on_activate(GtkApplication* app, gpointer /*user_data*/) {
    load_css();

    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "BeaverKiosk GTK");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);

    GtkWidget* root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
    gtk_widget_set_margin_top(root_box, 24);
    gtk_widget_set_margin_bottom(root_box, 24);
    gtk_widget_set_margin_start(root_box, 32);
    gtk_widget_set_margin_end(root_box, 32);
    gtk_window_set_child(GTK_WINDOW(window), root_box);

    GtkWidget* heading = gtk_label_new("BeaverKiosk — Local Toolkit");
    gtk_style_context_add_class(gtk_widget_get_style_context(heading), "heading-title");
    gtk_widget_set_halign(heading, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(heading), 0.0f);
    gtk_box_append(GTK_BOX(root_box), heading);

    GtkWidget* subtitle = gtk_label_new("Launch desktop utilities directly without any web server.");
    gtk_style_context_add_class(gtk_widget_get_style_context(subtitle), "app-description");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0f);
    gtk_box_append(GTK_BOX(root_box), subtitle);

    GtkWidget* flowbox = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), 18);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), 18);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flowbox), 3);
    gtk_box_append(GTK_BOX(root_box), flowbox);

    AppState* state = g_new0(AppState, 1);

    for (const auto& tile : kApps) {
        GtkWidget* tile_widget = build_tile_widget(tile, state);
        gtk_flow_box_append(GTK_FLOW_BOX(flowbox), tile_widget);
    }

    GtkWidget* status_frame = gtk_frame_new(nullptr);
    gtk_style_context_add_class(gtk_widget_get_style_context(status_frame), "status-frame");
    gtk_widget_set_margin_top(status_frame, 12);
    gtk_box_append(GTK_BOX(root_box), status_frame);

    GtkWidget* status_label = gtk_label_new("Select an application to launch locally.");
    gtk_widget_set_margin_top(status_label, 12);
    gtk_widget_set_margin_bottom(status_label, 12);
    gtk_widget_set_margin_start(status_label, 16);
    gtk_widget_set_margin_end(status_label, 16);
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0f);
    gtk_frame_set_child(GTK_FRAME(status_frame), status_label);

    state->status_label = GTK_LABEL(status_label);

    g_object_set_data_full(G_OBJECT(window), "app-state", state, [](gpointer data) {
        AppState* state_ptr = static_cast<AppState*>(data);
        g_free(state_ptr);
    });

    gtk_widget_show(window);
}

int main(int argc, char* argv[]) {
    g_set_application_name("BeaverKiosk GTK");

    g_autoptr(GtkApplication) app = gtk_application_new("com.beaver.kiosk", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);

    return g_application_run(G_APPLICATION(app), argc, argv);
}
