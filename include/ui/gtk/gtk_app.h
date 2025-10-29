#pragma once

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <string>

#if defined(HAVE_GSTREAMER)
#include <gst/gst.h>
#endif

#include "core/app_manager.h"

class GtkApp {
public:
    explicit GtkApp(AppManager& manager);
    int run(int argc, char** argv);

private:
    static void on_activate(GtkApplication* application, gpointer user_data);
    static gboolean on_decide_policy(WebKitWebView* web_view, WebKitPolicyDecision* decision,
                                     WebKitPolicyDecisionType decision_type, gpointer user_data);
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
    void ensure_remote_navigation_controls(WebKitWebView* web_view);
    void remove_remote_navigation_controls(WebKitWebView* web_view);
    void handle_remote_go_home();
    void set_camera_active(bool active);

    AppManager& manager_;
    WebKitWebView* web_view_ = nullptr;
    GtkWidget* overlay_root_ = nullptr;
#if defined(HAVE_GSTREAMER)
    void ensure_gstreamer_initialized();
    void ensure_camera_overlay();
    void start_camera_pipeline();
    void stop_camera_pipeline();
    void set_camera_status(const std::string& message);
    void attach_camera_widget_from_sink(GstElement* sink);
    GstElement* create_camera_sink();
    void attach_camera_bus();
    void detach_camera_bus();
    static void on_application_shutdown(GApplication* application, gpointer user_data);
    static void on_camera_bus_error(GstBus* bus, GstMessage* message, gpointer user_data);
    static void on_camera_bus_eos(GstBus* bus, GstMessage* message, gpointer user_data);
    static void on_camera_bus_state_changed(GstBus* bus, GstMessage* message, gpointer user_data);
    static gboolean on_camera_retry_timeout(gpointer user_data);
    static void on_camera_retry_clicked(GtkButton* button, gpointer user_data);
    bool prepare_camera_navigation(Language language);
    void begin_camera_proxy_wait(Language language);
    bool camera_proxy_available(const CctvConfig& config) const;
    void show_camera_overlay_message(const std::string& message, bool show_retry_button);
    void cancel_camera_retry_timeout();
    void schedule_camera_retry(unsigned int delay_seconds);
    void attempt_camera_reconnect();

    GtkWidget* camera_overlay_ = nullptr;
    GtkWidget* camera_frame_ = nullptr;
    GtkWidget* camera_status_label_ = nullptr;
    GtkWidget* camera_video_widget_ = nullptr;
    GtkWidget* camera_retry_button_ = nullptr;
    GstElement* camera_pipeline_ = nullptr;
    bool gstreamer_initialized_ = false;
    bool camera_requested_active_ = false;
    bool pending_camera_navigation_ = false;
    Language pending_camera_language_ = Language::English;
    Language current_camera_language_ = Language::English;
    guint camera_retry_timeout_id_ = 0;
    unsigned int camera_retry_attempts_ = 0;
#endif
};
