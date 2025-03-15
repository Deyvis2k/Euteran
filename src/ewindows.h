#ifndef EWINDOWS_H
#define EWINDOWS_H

#include "gio/gio.h"
#include <gtk/gtk.h>


void on_file_opened(GtkFileDialog *dialog, GAsyncResult *res, gpointer user_data);
void select_file(GtkWidget *button, gpointer user_data);
void animate_label(const char *music_name, gpointer user_data);
void create_new_window(GtkWidget *window_parent, gpointer user_data);


#endif
