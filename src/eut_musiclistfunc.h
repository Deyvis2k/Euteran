#pragma once


#include <gtk/gtk.h>
#include "eut_utils.h"
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
    EuteranMainObject *widgets_object,
    PlayMusicFunc   play_selected_music
);

void clear_container_children(GtkWidget *container);   

gboolean add_music_to_list(
    gpointer        user_data,
    const gchar     *path,
    PlayMusicFunc   play_selected_music
);
