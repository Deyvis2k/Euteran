#pragma once


#include "gio/gio.h"
#include "eut_main_object.h"
#include <gtk/gtk.h>




void on_folder_open(GtkFileDialog *dialog, GAsyncResult *res, gpointer user_data);
void select_folder(EuteranMainObject *wd, GtkButton *button);
