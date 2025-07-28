#pragma once


#include "gio/gio.h"
#include <gtk/gtk.h>




void on_folder_open(GtkFileDialog *dialog, GAsyncResult *res, gpointer user_data);
void select_folder(GtkWidget *button, gpointer user_data);
