#include <gtk/gtk.h>
#include "../include/html_generator.h" // Include the header for HTML generation
#include <iostream>                     // For basic output if needed

// Activate function: Called when the GTK application starts
static void activate (GtkApplication* app, gpointer user_data) {
  GtkWidget *window;
  GtkWidget *label; // Example widget

  // Create the main application window
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Beaver Kiosk");
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);

  // --- Example Usage of HTML Generator ---
  // You would typically use this HTML in a GtkWebView or serve it via HTTP.
  // For now, let's just generate it and print a message.
  std::vector<AppTile> apps = get_sample_apps();
  std::string menu_html = generate_menu_page_html(apps);
  std::string menu_json = generate_menu_json(apps);

  // You could print the HTML/JSON to the console for verification:
  // std::cout << "Generated HTML:\n" << menu_html << std::endl;
  // std::cout << "\nGenerated JSON:\n" << menu_json << std::endl;

  // Example: Add a simple label to the window
  label = gtk_label_new ("GTK Window Initialized. Menu HTML/JSON generated (check console if uncommented).");
  gtk_window_set_child(GTK_WINDOW(window), label); // Use gtk_window_set_child for GTK4

  // Show the window
  gtk_window_present(GTK_WINDOW(window));
}

// Main function: Entry point of the application
int main(int argc, char **argv) {
  GtkApplication *app;
  int status;

  // Create a new GTK application instance
  app = gtk_application_new ("org.example.beaverkiosk", G_APPLICATION_DEFAULT_FLAGS);

  // Connect the "activate" signal to our activate function
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  // Run the application's main loop
  status = g_application_run (G_APPLICATION (app), argc, argv);

  // Clean up the application object
  g_object_unref (app);

  // Return the application's exit status
  return status;
}
