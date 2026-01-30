#include "term.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <pwd.h>
#include <vector>

static bool starts_with(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && std::equal(p.begin(), p.end(), s.begin());
}

static std::string uri_to_path(const std::string& s) {
    if (starts_with(s, "file://")) {
        char* p = g_filename_from_uri(s.c_str(), nullptr, nullptr);
        if (p) {
            std::string out = p;
            g_free(p);
            return out;
        }
    }
    return s;
}

static std::string user_shell() {
    const char* sh = getenv("SHELL");
    if (sh && *sh) return sh;
    passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_shell && *pw->pw_shell) return pw->pw_shell;
    return "/bin/bash";
}

static void parse_rgba(const char* hex, GdkRGBA* out) {
    if (!gdk_rgba_parse(out, hex)) {
        out->red = out->green = out->blue = 0.0;
        out->alpha = 1.0;
    }
}

static void set_grayscale_palette(VteTerminal* term) {
    GdkRGBA fg, bg;
    parse_rgba("#d0d0d0", &fg);
    parse_rgba("#050505", &bg);

    const char* hex[16] = {
        "#000000","#202020","#404040","#606060",
        "#808080","#9a9a9a","#bcbcbc","#dcdcdc",
        "#101010","#303030","#505050","#707070",
        "#909090","#b0b0b0","#d0d0d0","#ffffff"
    };

    GdkRGBA palette[16];
    for (int i = 0; i < 16; ++i) parse_rgba(hex[i], &palette[i]);
    vte_terminal_set_colors(term, &fg, &bg, palette, 16);
}

static void terminal_notice(VteTerminal* term, const std::string& msg) {
    std::string m = "\r\n[COLOSSUS] " + msg + "\r\n";
    vte_terminal_feed(term, m.c_str(), (gssize)m.size());
}

static void on_spawn_ready(VteTerminal* term, GPid pid, GError* err, gpointer) {
    (void)pid;
    if (err) terminal_notice(term, err->message ? err->message : "spawn error");
}

static std::vector<std::string> build_spawn_argv(const std::string& exec_cmd) {
    std::string sh = user_shell();
    if (exec_cmd.empty()) return { sh };
    return { sh, "-lc", exec_cmd };
}

GtkWidget* colossus_term_new_scrolled(VteTerminal** out_term,
                                      const std::string& cwd_in,
                                      const std::string& exec_cmd)
{
    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    GtkWidget* w = vte_terminal_new();
    VteTerminal* term = VTE_TERMINAL(w);

    if (out_term) *out_term = term;

    vte_terminal_set_scrollback_lines(term, 10000);
    vte_terminal_set_scroll_on_keystroke(term, TRUE);
    vte_terminal_set_scroll_on_output(term, FALSE);
    vte_terminal_set_audible_bell(term, FALSE);

    set_grayscale_palette(term);

    PangoFontDescription* fd =
        pango_font_description_from_string("JetBrainsMono Nerd Font 12");
    vte_terminal_set_font(term, fd);
    pango_font_description_free(fd);

    std::string cwd = cwd_in.empty() ? std::string(g_get_home_dir())
                                     : uri_to_path(cwd_in);

    auto args = build_spawn_argv(exec_cmd);
    std::vector<char*> argv_spawn;
    argv_spawn.reserve(args.size() + 1);
    for (auto& s : args) argv_spawn.push_back(const_cast<char*>(s.c_str()));
    argv_spawn.push_back(nullptr);

    vte_terminal_spawn_async(
        term,
        VTE_PTY_DEFAULT,
        cwd.c_str(),
        argv_spawn.data(),
        nullptr,
        G_SPAWN_SEARCH_PATH,
        nullptr, nullptr,
        nullptr,
        -1,
        nullptr,
        on_spawn_ready,
        nullptr
    );

    gtk_container_add(GTK_CONTAINER(scrolled), w);
    return scrolled;
}
