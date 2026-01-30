#ifndef MONO_BROWSER_H
#define MONO_BROWSER_H

#include <string>
#include <vector>

extern "C" {
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
}

class Browser {
public:
    explicit Browser(GtkApplication* app);
    ~Browser();

    void show();
    GtkWidget* window_widget() const { return window_; }

private:
    enum class TabKind { Web, Term };

    struct Tab {
        TabKind kind = TabKind::Web;

        GtkWidget* scrolled = nullptr;   // container added to notebook
        GtkWidget* label = nullptr;      // tab label

        WebKitWebView* view = nullptr;   // only for Web tabs
        GtkWidget* term = nullptr;       // only for Term tabs (VTE widget)
    };

    GtkApplication* app_ = nullptr;
    GtkWidget* window_ = nullptr;
    GtkWidget* notebook_ = nullptr;
    GtkWidget* bottom_bar_ = nullptr;
    GtkWidget* url_entry_ = nullptr;

    GtkWidget* back_button_ = nullptr;
    GtkWidget* forward_button_ = nullptr;
    GtkWidget* home_button_ = nullptr;
    GtkWidget* new_tab_button_ = nullptr;
    GtkWidget* term_tab_button_ = nullptr;

    WebKitSettings* settings_ = nullptr;

    std::vector<Tab> tabs_;
    int current_tab_ = -1;

    void setup_ui();

    void new_web_tab(const std::string& uri);
    void new_term_tab(const std::string& cwd = "", const std::string& exec_cmd = "");

    WebKitWebView* current_view();
    Tab* current_tab();

    void close_current_tab();

    void go_back();
    void go_forward();
    void go_home();
    void reload();

    void on_url_activate();
    void on_tab_switched(guint page);
    void on_load_changed(WebKitWebView* view, WebKitLoadEvent event);
    gboolean on_key_press(GdkEventKey* event);

    static gboolean s_key_press(GtkWidget*, GdkEventKey* event, gpointer user_data);
    static void s_url_activate(GtkEntry*, gpointer user_data);
    static void s_tab_switched(GtkNotebook*, GtkWidget*, guint page, gpointer user_data);
    static void s_load_changed(WebKitWebView* view, WebKitLoadEvent event, gpointer user_data);
};

#endif
