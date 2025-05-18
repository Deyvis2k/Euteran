#include "e_widgets.h"
#include "constants.h"
#include "gtk/gtk.h"
#include "utils.h"

void clear_container_children(GtkWidget *container) {
    GtkWidget *child = gtk_widget_get_first_child(container);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);

        if(child != NULL)
            gtk_widget_unparent(child); 
        child = next;
    }
}

char *cast_simple_double_to_string(double value) {
    if (value < 0){
        fprintf(stderr, "Error allocating memory in value\n");
        return NULL;
    }

    char* buffer = malloc(10);  
    if (!buffer){
        fprintf(stderr, "Error allocating memory\n");
        return NULL;
    }

    snprintf(buffer, 10, "%.0f", value);
    return buffer;
}

void create_music_list(const gchar *path, WidgetsData *widgets_data, GtkWidget *music_holder, PlayMusicFunc play_selected_music) {
    if (!widgets_data || !GTK_IS_LIST_BOX(widgets_data->list_box)) {
        g_print("Erro: widgets_data ou seus componentes são inválidos\n");
        return;
    }

    music_list_t new_music_list = list_files_musics(path);
    if (new_music_list.musics == NULL) {
        g_print("Erro: falha ao listar músicas\n");
        return;
    }

    gtk_list_box_remove_all(GTK_LIST_BOX(widgets_data->list_box));

    for (size_t i = 0; i < new_music_list.count_size; i++) {
        if (new_music_list.musics[i].name == NULL) {
            printf(RED_COLOR "[ERROR] Música inválida no índice %zu\n" RESET_COLOR, i);
            continue;
        }

        GtkWidget *row = gtk_list_box_row_new();
        if (!row) {
            g_print("Erro: falha ao criar row\n");
            continue;
        }

        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_hexpand(row_box, TRUE);
        gtk_widget_add_css_class(row_box, "music_row");

        GtkWidget *label = gtk_label_new(new_music_list.musics[i].name);
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_widget_set_halign(label, GTK_ALIGN_BASELINE_CENTER);
        gtk_widget_set_hexpand(label, TRUE); 
        gtk_widget_add_css_class(label, "music_name_label");
        gtk_box_append(GTK_BOX(row_box), label);

        GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_vexpand(separator, TRUE);
        gtk_widget_add_css_class(separator, "separator_class");
        gtk_widget_set_halign(separator, GTK_ALIGN_END);
        gtk_box_append(GTK_BOX(row_box), separator);

        const char *duration_str = cast_double_to_string(new_music_list.musics[i].duration);
        GtkWidget *duration = gtk_label_new(duration_str ? duration_str : "0.0");        
        int offset = (85 - (strlen(duration_str) * 10)) >= 0 ? (85 - (strlen(duration_str) * 10)) : 0;
        offset = offset - 20;
        gtk_label_set_xalign(GTK_LABEL(duration), 1.0);
        gtk_widget_set_margin_end(duration, offset);
        gtk_box_append(GTK_BOX(row_box), duration);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
        gtk_list_box_insert(GTK_LIST_BOX(widgets_data->list_box), row, (gint)i);

        g_object_set_data(G_OBJECT(row), "music_name", new_music_list.musics[i].name);
        g_object_set_data(G_OBJECT(row), "music_duration",
                          cast_simple_double_to_string(new_music_list.musics[i].duration));
    }

    if (new_music_list.musics != NULL) {
        for (size_t i = 0; i < new_music_list.count_size; i++) {
            free(new_music_list.musics[i].name);
        }
        free(new_music_list.musics);
    }

    g_signal_handlers_disconnect_by_func(widgets_data->list_box, G_CALLBACK(play_selected_music), widgets_data);
    g_signal_connect(widgets_data->list_box, "row-activated", G_CALLBACK(play_selected_music), widgets_data);
}
