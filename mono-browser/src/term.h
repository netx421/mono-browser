#ifndef COLOSSUS_TERM_H
#define COLOSSUS_TERM_H

#include <string>

extern "C" {
#include <gtk/gtk.h>
#include <vte/vte.h>
}

GtkWidget* colossus_term_new_scrolled(VteTerminal** out_term,
                                      const std::string& cwd = "",
                                      const std::string& exec_cmd = "");

#endif
