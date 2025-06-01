#ifndef E_WIDGETS_H
#define E_WIDGETS_H

#include <gtk/gtk.h>
#include "utils.h"


void create_music_list(const gchar *path, WidgetsData *data, GtkWidget *grid_parent, PlayMusicFunc func);
void clear_container_children(GtkWidget *container);   
gboolean add_music_to_list(gpointer user_data, gchar *path, const gchar *uri, GFile *file, PlayMusicFunc play_selected_music);

#endif
