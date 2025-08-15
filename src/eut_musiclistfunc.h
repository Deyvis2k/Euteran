#pragma once


#include <gtk/gtk.h>
#include "eut_main_object.h"

typedef struct {
    GtkWidget *row_box;
    GtkWidget *row;
    GtkWidget *window_parent;
    GtkWidget *list_box;
    gchar     *path;
}EMusicContainer;


void create_music_list(
    const gchar     *path, 
    EuteranMainObject *main_object
);

gboolean add_music_to_list(
    gpointer        user_data,
    const gchar     *path
);

void 
create_music_row
(
    EuteranMainObject *main_object,
    GtkListBox        *list_box,
    const gchar       *music_filename,
    const gchar       *music_duration_raw,
    gdouble           music_duration
);

void
free_music_container(
    EMusicContainer *container
);
