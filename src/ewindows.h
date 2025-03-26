#ifndef EWINDOWS_H
#define EWINDOWS_H

#include "gio/gio.h"
#include <gtk/gtk.h>




void on_folder_open(GtkFileDialog *dialog, GAsyncResult *res, gpointer user_data);
void select_folder(GtkWidget *button, gpointer user_data);
#endif
