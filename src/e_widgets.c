#include "e_widgets.h"
#include "constants.h"
#include "gtk/gtk.h"
#include "utils.h"


void clear_container_children(GtkWidget *container) {
    GtkWidget *child = gtk_widget_get_first_child(container);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_widget_unparent(child); 
        child = next;
    }
}

static GtkWidget* find_row_by_name(GtkListBox *list_box, const char *name) {
    GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(list_box));
    while (child != NULL) {
        if (GTK_IS_LIST_BOX_ROW(child)) {
            GtkWidget *label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(child));
            if (label && GTK_IS_LABEL(label)) {
                const char *label_text = gtk_label_get_text(GTK_LABEL(label));
                if (label_text && strcmp(label_text, name) == 0) {
                    return child;
                }
            }
        }
        child = gtk_widget_get_next_sibling(child);
    }
    return NULL;
}

static GtkWidget* find_duration_by_index(GtkBox *duration_box, gint index) {
    GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(duration_box));
    gint i = 0;
    while (child != NULL) {
        if (i == index) {
            return child;
        }
        i++;
        child = gtk_widget_get_next_sibling(child);
    }
    return NULL;
}

void create_music_list(const gchar *path, WidgetsData *widgets_data, GtkWidget *grid, PlayMusicFunc play_selected_music) {
    if (path == NULL || strlen(path) == 0) {
        g_print("Path não pode ser nulo ou vazio\n");
        GtkWidget *existing_label = gtk_widget_get_first_child(grid);
        if (!existing_label) {
            GtkWidget *label = gtk_label_new("Nenhuma música encontrada");
            gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
            gtk_widget_set_visible(label, TRUE);
        }
        return;
    }

    if (!widgets_data || !GTK_IS_LIST_BOX(widgets_data->list_box) || !GTK_IS_BOX(widgets_data->duration_box)) {
        g_print("Erro: widgets_data ou seus componentes são inválidos\n");
        return;
    }

    gtk_widget_set_visible(widgets_data->list_box, FALSE);
    gtk_widget_set_visible(widgets_data->duration_box, FALSE);

    music_list_t new_music_list = list_files_musics(path); 

    GHashTable *existing_names = g_hash_table_new(g_str_hash, g_str_equal);
    GtkWidget *row_child = gtk_widget_get_first_child(widgets_data->list_box);
    while (row_child != NULL) {
        if (GTK_IS_LIST_BOX_ROW(row_child)) {
            GtkWidget *label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row_child));
            if (label && GTK_IS_LABEL(label)) {
                const char *name = gtk_label_get_text(GTK_LABEL(label));
                g_hash_table_insert(existing_names, g_strdup(name), row_child);
            }
        }
        row_child = gtk_widget_get_next_sibling(row_child);
    }

    for (size_t i = 0; i < new_music_list.count_size; i++) {
        if (new_music_list.musics[i].name == NULL || new_music_list.musics[i].duration <= 0) {
            printf(RED_COLOR"[ERROR] Música inválida no índice %zu\n" RESET_COLOR, i);
            continue;
        }

        GtkWidget *existing_row = find_row_by_name(GTK_LIST_BOX(widgets_data->list_box), new_music_list.musics[i].name);
        if (existing_row) {
            GtkWidget *duration_label = find_duration_by_index(GTK_BOX(widgets_data->duration_box), i);
            const char *new_duration_str = cast_double_to_string(new_music_list.musics[i].duration);
            if (duration_label && GTK_IS_LABEL(duration_label)) {
                const char *current_duration = gtk_label_get_text(GTK_LABEL(duration_label));
                if (strcmp(current_duration, new_duration_str ? new_duration_str : "0.0") != 0) {
                    gtk_label_set_text(GTK_LABEL(duration_label), new_duration_str ? new_duration_str : "0.0");
                }
            }
            g_hash_table_remove(existing_names, new_music_list.musics[i].name); 
        } else {
            GtkWidget *row = gtk_list_box_row_new();
            if (!row) {
                g_print("Erro: falha ao criar row\n");
                continue;
            }
            GtkWidget *label = gtk_label_new(new_music_list.musics[i].name);
            if (!label) {
                g_print("Erro: falha ao criar label\n");
                gtk_widget_unparent(row);
                continue;
            }
            const char *duration_str = cast_double_to_string(new_music_list.musics[i].duration);
            GtkWidget *duration = gtk_label_new(duration_str ? duration_str : "0.0");
            if (!duration) {
                g_print("Erro: falha ao criar duration\n");
                gtk_widget_unparent(row);
                gtk_widget_unparent(label);
                continue;
            }
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);

            gtk_box_append(GTK_BOX(widgets_data->duration_box), duration);
            gtk_list_box_insert(GTK_LIST_BOX(widgets_data->list_box), row, (gint)i);

        }
    }

    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, existing_names);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        const char *name = (const char *)key;
        GtkWidget *row = (GtkWidget *)value;

        gtk_widget_unparent(row);

        GtkWidget *duration = find_duration_by_index(GTK_BOX(widgets_data->duration_box), gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)));
        if (duration) {
            gtk_widget_unparent(duration);
        }
    }
    g_hash_table_destroy(existing_names);

    if (new_music_list.musics != NULL) {
        for (size_t i = 0; i < new_music_list.count_size; i++) {
            free(new_music_list.musics[i].name);
        }
        free(new_music_list.musics);
    }

    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
    }

    gtk_widget_set_visible(widgets_data->list_box, TRUE);
    gtk_widget_set_visible(widgets_data->duration_box, TRUE);

    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
    }

    g_signal_handlers_disconnect_by_func(widgets_data->list_box, G_CALLBACK(play_selected_music), widgets_data);
    g_signal_connect(widgets_data->list_box, "row-activated", G_CALLBACK(play_selected_music), widgets_data);
}
