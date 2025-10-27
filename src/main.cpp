#include <gtk/gtk.h>
#include <string>
#include <vector>

#include "html_generator.h"

namespace {

const char kAppCss[] = R"css(
window {
  background-image: radial-gradient(circle at 20% 20%, #1f2937, #0b1120 70%);
  background-color: #0f172a;
  color: #f9fafb;
  font-family: "Inter", "Cantarell", "Segoe UI", sans-serif;
}

.root {
  padding: 32px;
  background-color: transparent;
}

.menu-root {
  max-width: 960px;
  margin-left: auto;
  margin-right: auto;
}

.menu-header {
  padding-bottom: 16px;
}

.menu-header-title {
  font-size: 32px;
  font-weight: 600;
  color: #f9fafb;
  margin-bottom: 6px;
}

.menu-header-subtitle {
  font-size: 14px;
  color: rgba(229, 231, 235, 0.7);
}

.menu-grid {
  padding-top: 8px;
}

.app-tile {
  background-color: rgba(17, 24, 39, 0.85);
  border: 2px solid rgba(255, 255, 255, 0.04);
  border-radius: 20px;
  padding: 18px 16px 16px 16px;
  min-width: 160px;
  min-height: 140px;
  color: inherit;
}

.app-tile:hover {
  border-color: rgba(255, 255, 255, 0.18);
}

.app-tile__icon {
  font-size: 42px;
  font-weight: 300;
  margin-bottom: 12px;
}

.app-tile__name {
  font-size: 16px;
  font-weight: 600;
}

.app-tile__icon,
.app-tile__name {
  color: inherit;
}

.app-tile--violet {
  background-image: linear-gradient(135deg, rgba(124, 58, 237, 0.16), rgba(129, 140, 248, 0.2));
}

.app-tile--cyan {
  background-image: linear-gradient(135deg, rgba(6, 182, 212, 0.16), rgba(34, 211, 238, 0.2));
}

.app-tile--amber {
  background-image: linear-gradient(135deg, rgba(217, 119, 6, 0.18), rgba(251, 191, 36, 0.2));
}

.app-tile--red {
  background-image: linear-gradient(135deg, rgba(239, 68, 68, 0.18), rgba(248, 113, 113, 0.2));
}

.app-tile--green {
  background-image: linear-gradient(135deg, rgba(34, 197, 94, 0.18), rgba(74, 222, 128, 0.2));
}

.menu-footer {
  font-size: 12px;
  color: rgba(229, 231, 235, 0.55);
  margin-top: 24px;
}
)css";

void attach_css() {
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(provider, kAppCss);

  if (GdkDisplay *display = gdk_display_get_default(); display != nullptr) {
    gtk_style_context_add_provider_for_display(
        display, GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }

  g_object_unref(provider);
}

GtkWidget *create_app_tile(const AppTile &app) {
  GtkWidget *button = gtk_button_new();
  gtk_widget_add_css_class(button, "app-tile");

  std::string accent_class = "app-tile--" + app.accent;
  gtk_widget_add_css_class(button, accent_class.c_str());

  GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign(content, GTK_ALIGN_START);
  gtk_widget_set_valign(content, GTK_ALIGN_START);
  gtk_box_set_spacing(GTK_BOX(content), 8);

  gchar initial_str[2] = {'A', '\0'};
  if (!app.name.empty()) {
    initial_str[0] = g_ascii_toupper(app.name.front());
  }
  GtkWidget *icon_label = gtk_label_new(initial_str);
  gtk_widget_add_css_class(icon_label, "app-tile__icon");

  GtkWidget *name_label = gtk_label_new(app.name.c_str());
  gtk_widget_add_css_class(name_label, "app-tile__name");
  gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);

  gtk_box_append(GTK_BOX(content), icon_label);
  gtk_box_append(GTK_BOX(content), name_label);

  gtk_button_set_child(GTK_BUTTON(button), content);
  gtk_widget_set_hexpand(button, TRUE);
  gtk_widget_set_vexpand(button, FALSE);

  return button;
}

GtkWidget *build_menu_grid(const std::vector<AppTile> &apps) {
  GtkWidget *flow = gtk_flow_box_new();
  gtk_widget_add_css_class(flow, "menu-grid");
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow), GTK_SELECTION_NONE);
  gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flow), 20);
  gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flow), 20);
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow), 3);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow), 4);
  gtk_widget_set_hexpand(flow, TRUE);
  gtk_widget_set_vexpand(flow, TRUE);
  gtk_widget_set_halign(flow, GTK_ALIGN_CENTER);

  for (const auto &app : apps) {
    GtkWidget *tile = create_app_tile(app);
    gtk_flow_box_insert(GTK_FLOW_BOX(flow), tile, -1);
  }

  return flow;
}

} // namespace

// Activate function: Called when the GTK application starts
static void activate(GtkApplication *app, gpointer /*user_data*/) {
  attach_css();

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Beaver Kiosk");
  gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);

  GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
  gtk_widget_add_css_class(root, "root");
  gtk_widget_add_css_class(root, "menu-root");
  gtk_widget_set_margin_top(root, 24);
  gtk_widget_set_margin_bottom(root, 24);
  gtk_widget_set_margin_start(root, 32);
  gtk_widget_set_margin_end(root, 32);
  gtk_widget_set_halign(root, GTK_ALIGN_CENTER);

  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_add_css_class(header, "menu-header");
  gtk_widget_set_halign(header, GTK_ALIGN_CENTER);

  GtkWidget *title = gtk_label_new("BeaverKiosk");
  gtk_widget_add_css_class(title, "menu-header-title");
  gtk_label_set_xalign(GTK_LABEL(title), 0.5);

  GtkWidget *subtitle = gtk_label_new(
      "Unified application launcher • C++ Edition");
  gtk_widget_add_css_class(subtitle, "menu-header-subtitle");
  gtk_label_set_xalign(GTK_LABEL(subtitle), 0.5);

  gtk_box_append(GTK_BOX(header), title);
  gtk_box_append(GTK_BOX(header), subtitle);

  std::vector<AppTile> apps = get_sample_apps();
  GtkWidget *grid = build_menu_grid(apps);

  GtkWidget *footer = gtk_label_new(
      "C++ • Native HTTP Server • Beaver Suite ©2025");
  gtk_widget_add_css_class(footer, "menu-footer");
  gtk_label_set_xalign(GTK_LABEL(footer), 0.5);
  gtk_widget_set_halign(footer, GTK_ALIGN_CENTER);

  gtk_box_append(GTK_BOX(root), header);
  gtk_box_append(GTK_BOX(root), grid);
  gtk_box_append(GTK_BOX(root), footer);

  gtk_window_set_child(GTK_WINDOW(window), root);
  gtk_window_present(GTK_WINDOW(window));
}

// Main function: Entry point of the application
int main(int argc, char **argv) {
  // Create a new GTK application instance
  GtkApplication *app =
      gtk_application_new("org.example.beaverkiosk", G_APPLICATION_DEFAULT_FLAGS);

  // Connect the "activate" signal to our activate function
  g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);

  // Run the application's main loop
  int status = g_application_run(G_APPLICATION(app), argc, argv);

  // Clean up the application object
  g_object_unref(app);

  // Return the application's exit status
  return status;
}
