#include "browser.h"
#include "term.h"

#include <gdk/gdkkeysyms.h>

static const char* HOMEPAGE = "https://telehack.com/";

static WebKitUserContentManager* make_mono_content_manager()
{
    WebKitUserContentManager* mgr = webkit_user_content_manager_new();

    const char* mono_css =
        "html{"
        "  -webkit-filter:grayscale(100%) !important;"
        "  filter:grayscale(100%) !important;"
        "  background:#000 !important;"
        "}"
        "img,video,canvas,svg,iframe{"
        "  -webkit-filter:grayscale(100%) !important;"
        "  filter:grayscale(100%) !important;"
        "}"
        "*{"
        "  color-scheme:dark;"
        "}";

    WebKitUserStyleSheet* sheet = webkit_user_style_sheet_new(
        mono_css,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_STYLE_LEVEL_USER,
        nullptr,
        nullptr
    );

    webkit_user_content_manager_add_style_sheet(mgr, sheet);
    webkit_user_style_sheet_unref(sheet);

    return mgr;
}

Browser::Browser(GtkApplication* app)
    : app_(app)
{
    settings_ = webkit_settings_new();

    webkit_settings_set_user_agent(
        settings_,
        "COLOSSUS-NAN/2.0 (COLOSSUS: NETWORK ACCESS NODE; PROJECT CN-007; TELEHACK) "
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/605.1 (KHTML, like Gecko) Safari/605.1"
    );

    setup_ui();
    new_web_tab(HOMEPAGE);
}

Browser::~Browser()
{
    if (settings_) {
        g_object_unref(settings_);
        settings_ = nullptr;
    }

    if (window_) {
        gtk_widget_destroy(window_);
        window_ = nullptr;
    }
}

void Browser::setup_ui()
{
    window_ = gtk_application_window_new(app_);
    gtk_window_set_title(GTK_WINDOW(window_), "COLOSSUS: NETWORK ACCESS NODE");
    gtk_window_set_default_size(GTK_WINDOW(window_), 1100, 700);

    g_signal_connect(window_, "key-press-event", G_CALLBACK(Browser::s_key_press), this);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window_), vbox);

    notebook_ = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook_), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), notebook_, TRUE, TRUE, 0);
    g_signal_connect(notebook_, "switch-page", G_CALLBACK(Browser::s_tab_switched), this);

    bottom_bar_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(bottom_bar_, 6);
    gtk_widget_set_margin_end(bottom_bar_, 6);
    gtk_widget_set_margin_bottom(bottom_bar_, 6);
    gtk_box_pack_end(GTK_BOX(vbox), bottom_bar_, FALSE, FALSE, 0);

    back_button_ = gtk_button_new_with_label("<");
    forward_button_ = gtk_button_new_with_label(">");
    home_button_ = gtk_button_new_with_label("HOME");
    new_tab_button_ = gtk_button_new_with_label("+");
    term_tab_button_ = gtk_button_new_with_label(">_");

    g_signal_connect(back_button_, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer d) { static_cast<Browser*>(d)->go_back(); }), this);
    g_signal_connect(forward_button_, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer d) { static_cast<Browser*>(d)->go_forward(); }), this);
    g_signal_connect(home_button_, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer d) { static_cast<Browser*>(d)->go_home(); }), this);
    g_signal_connect(new_tab_button_, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer d) { static_cast<Browser*>(d)->new_web_tab(HOMEPAGE); }), this);
    g_signal_connect(term_tab_button_, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer d) { static_cast<Browser*>(d)->new_term_tab(); }), this);

    gtk_box_pack_start(GTK_BOX(bottom_bar_), back_button_, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bottom_bar_), forward_button_, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bottom_bar_), home_button_, FALSE, FALSE, 6);

    url_entry_ = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(url_entry_), "open host / search");
    gtk_widget_set_hexpand(url_entry_, TRUE);
    g_signal_connect(url_entry_, "activate", G_CALLBACK(Browser::s_url_activate), this);
    gtk_box_pack_start(GTK_BOX(bottom_bar_), url_entry_, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(bottom_bar_), term_tab_button_, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bottom_bar_), new_tab_button_, FALSE, FALSE, 0);
}

void Browser::show()
{
    if (window_) gtk_widget_show_all(window_);
}

Browser::Tab* Browser::current_tab()
{
    if (current_tab_ < 0 || current_tab_ >= (int)tabs_.size()) return nullptr;
    return &tabs_[current_tab_];
}

WebKitWebView* Browser::current_view()
{
    Tab* t = current_tab();
    if (!t || t->kind != TabKind::Web) return nullptr;
    return t->view;
}

void Browser::new_web_tab(const std::string& uri)
{
    Tab t;
    t.kind = TabKind::Web;

    t.scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(t.scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    WebKitUserContentManager* mgr = make_mono_content_manager();
    t.view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(mgr));
    g_object_unref(mgr);

    webkit_web_view_set_settings(t.view, settings_);

    GdkRGBA black = {0, 0, 0, 1};
    webkit_web_view_set_background_color(t.view, &black);

    g_signal_connect(t.view, "load-changed", G_CALLBACK(Browser::s_load_changed), this);

    gtk_container_add(GTK_CONTAINER(t.scrolled), GTK_WIDGET(t.view));

    t.label = gtk_label_new("NODE");

    gint page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook_), t.scrolled, t.label);
    tabs_.push_back(t);

    gtk_widget_show_all(t.scrolled);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_), page);
    current_tab_ = page;

    if (!uri.empty()) webkit_web_view_load_uri(t.view, uri.c_str());
}

void Browser::new_term_tab(const std::string& cwd, const std::string& exec_cmd)
{
    Tab t;
    t.kind = TabKind::Term;

    VteTerminal* term = nullptr;
    t.scrolled = colossus_term_new_scrolled(&term, cwd, exec_cmd);
    t.term = GTK_WIDGET(term);

    t.label = gtk_label_new("TERM");

    gint page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook_), t.scrolled, t.label);
    tabs_.push_back(t);

    gtk_widget_show_all(t.scrolled);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_), page);
    current_tab_ = page;

    if (term) {
        g_signal_connect(term, "window-title-changed",
            G_CALLBACK(+[](VteTerminal* tterm, gpointer data) {
                GtkWidget* label = GTK_WIDGET(data);
                char* tt = nullptr;
                g_object_get(tterm, "window-title", &tt, nullptr);
                const char* title = (tt && *tt) ? tt : "TERM";
                gtk_label_set_text(GTK_LABEL(label), title);
                if (tt) g_free(tt);
            }),
            t.label
        );
    }
}

void Browser::close_current_tab()
{
    int idx = current_tab_;
    if (idx < 0 || idx >= (int)tabs_.size()) return;

    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_), idx);
    tabs_.erase(tabs_.begin() + idx);

    if (tabs_.empty()) {
        gtk_widget_destroy(window_);
        return;
    }

    if (idx >= (int)tabs_.size()) idx = (int)tabs_.size() - 1;
    current_tab_ = idx;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_), current_tab_);
}

void Browser::go_back()
{
    auto* v = current_view();
    if (v && webkit_web_view_can_go_back(v)) webkit_web_view_go_back(v);
}

void Browser::go_forward()
{
    auto* v = current_view();
    if (v && webkit_web_view_can_go_forward(v)) webkit_web_view_go_forward(v);
}

void Browser::go_home()
{
    auto* v = current_view();
    if (v) webkit_web_view_load_uri(v, HOMEPAGE);
}

void Browser::reload()
{
    auto* v = current_view();
    if (v) webkit_web_view_reload(v);
}

void Browser::on_url_activate()
{
    const gchar* text = gtk_entry_get_text(GTK_ENTRY(url_entry_));
    if (!text || !*text) return;

    std::string s(text);
    std::string uri;

    if (s.find("://") != std::string::npos) {
        uri = s;
    } else if (s.rfind("about:", 0) == 0 || s.rfind("file:", 0) == 0) {
        uri = s;
    } else if (s.rfind("www.", 0) == 0) {
        uri = "https://" + s;
    } else if (s.find('.') != std::string::npos && s.find(' ') == std::string::npos) {
        uri = "https://" + s;
    } else {
        uri = "https://duckduckgo.com/?q=";
        for (char c : s) uri += (c == ' ') ? '+' : c;
    }

    if (!current_view()) {
        new_web_tab(uri);
        return;
    }

    auto* v = current_view();
    if (v) webkit_web_view_load_uri(v, uri.c_str());
}

void Browser::on_tab_switched(guint page)
{
    current_tab_ = (int)page;

    Tab* t = current_tab();
    if (!t) return;

    if (t->kind == TabKind::Web && t->view && url_entry_) {
        const gchar* uri = webkit_web_view_get_uri(t->view);
        gtk_entry_set_text(GTK_ENTRY(url_entry_), uri ? uri : "");
    } else if (url_entry_) {
        gtk_entry_set_text(GTK_ENTRY(url_entry_), "");
    }
}

void Browser::on_load_changed(WebKitWebView* view, WebKitLoadEvent event)
{
    if (event != WEBKIT_LOAD_FINISHED) return;

    const gchar* title = webkit_web_view_get_title(view);
    if (!title || !*title) title = "NODE";

    const gchar* uri = webkit_web_view_get_uri(view);

    for (auto& t : tabs_) {
        if (t.kind == TabKind::Web && t.view == view && t.label) {
            gtk_label_set_text(GTK_LABEL(t.label), title);
            break;
        }
    }

    if (view == current_view() && url_entry_) {
        gtk_entry_set_text(GTK_ENTRY(url_entry_), uri ? uri : "");
    }
}

gboolean Browser::on_key_press(GdkEventKey* e)
{
    if (!e) return FALSE;

    bool ctrl  = (e->state & GDK_CONTROL_MASK);
    bool shift = (e->state & GDK_SHIFT_MASK);

    if (e->keyval == GDK_KEY_F4) {
        close_current_tab();
        return TRUE;
    }

    if (ctrl && shift && (e->keyval == GDK_KEY_t || e->keyval == GDK_KEY_T)) {
        new_term_tab();
        return TRUE;
    }

    if (ctrl && (e->keyval == GDK_KEY_t || e->keyval == GDK_KEY_T)) {
        new_web_tab(HOMEPAGE);
        return TRUE;
    }

    if (ctrl && (e->keyval == GDK_KEY_w || e->keyval == GDK_KEY_W)) {
        close_current_tab();
        return TRUE;
    }

    if (ctrl && e->keyval == GDK_KEY_l) {
        gtk_widget_grab_focus(url_entry_);
        gtk_editable_select_region(GTK_EDITABLE(url_entry_), 0, -1);
        return TRUE;
    }

    if ((e->state & GDK_MOD1_MASK) && e->keyval == GDK_KEY_Left) {
        go_back();
        return TRUE;
    }
    if ((e->state & GDK_MOD1_MASK) && e->keyval == GDK_KEY_Right) {
        go_forward();
        return TRUE;
    }

    if (ctrl && e->keyval == GDK_KEY_F1) { new_web_tab(HOMEPAGE); return TRUE; }
    if (ctrl && e->keyval == GDK_KEY_F2) { reload(); return TRUE; }
    if (ctrl && e->keyval == GDK_KEY_F3) { go_home(); return TRUE; }

    return FALSE;
}

gboolean Browser::s_key_press(GtkWidget*, GdkEventKey* e, gpointer d)
{
    return static_cast<Browser*>(d)->on_key_press(e);
}

void Browser::s_url_activate(GtkEntry*, gpointer d)
{
    static_cast<Browser*>(d)->on_url_activate();
}

void Browser::s_tab_switched(GtkNotebook*, GtkWidget*, guint p, gpointer d)
{
    static_cast<Browser*>(d)->on_tab_switched(p);
}

void Browser::s_load_changed(WebKitWebView* v, WebKitLoadEvent e, gpointer d)
{
    static_cast<Browser*>(d)->on_load_changed(v, e);
}
