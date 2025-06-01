#include "e_widgets.h"
#include "constants.h"
#include "gtk/gtk.h"
#include "utils.h"
#include "audio.h"

void clear_container_children(GtkWidget *container) {
    GtkWidget *child = gtk_widget_get_first_child(container);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);

        if(child != NULL)
            gtk_widget_unparent(child); 
        child = next;
    }
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

        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
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
        gtk_widget_add_css_class(duration, "music_name_label");
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

gboolean add_music_to_list(gpointer user_data, gchar *path, const gchar *uri, GFile *file, PlayMusicFunc play_selected_music) {
    gchar *clean_uri = NULL;
    GError *error = NULL;

    if (g_path_is_absolute(uri)) {
        clean_uri = g_strdup(uri);
    } else {
        clean_uri = g_filename_from_uri(uri, NULL, &error);
        if (clean_uri == NULL) {
            g_print(RED_COLOR "[ERROR] Erro ao decodificar URI: %s\n" RESET_COLOR, error->message);
            g_error_free(error);
            return FALSE;
        }
    }
    g_strchomp(clean_uri);

    GFile *gfile = g_file_new_for_path(clean_uri);
    if (!g_file_query_exists(gfile, NULL)) {
        g_print(RED_COLOR "[ERROR] Arquivo %s não existe\n" RESET_COLOR, clean_uri);
        g_object_unref(gfile);
        g_free(clean_uri);
        return FALSE;
    }
    g_object_unref(gfile);

    FILE *test_file = fopen(clean_uri, "r");
    if (!test_file) {
        g_print(RED_COLOR "[ERROR] Erro ao abrir arquivo %s: %s\n" RESET_COLOR, clean_uri, strerror(errno));
        g_free(clean_uri);
        return FALSE;
    }
    fclose(test_file);
    const char *ext = strrchr(clean_uri, '.');

    if (ext == NULL || *(ext + 1) == '\0') {
        g_print(RED_COLOR "[ERROR] Extensão inválida\n" RESET_COLOR);
        g_free(clean_uri);
        return FALSE;
    }

    ext++; 

    if (g_ascii_strcasecmp(ext, "mp3") != 0 && g_ascii_strcasecmp(ext, "ogg") != 0) {
        g_print("Extensão inválida: %s\n", ext);
        g_free(clean_uri);
        return FALSE;
    }

    const char *FILENAME = strrchr(clean_uri, '/') + 1;
    g_print("filename with clean uri: %s\n", FILENAME);

    gchar *link_path = g_strdup_printf("%s%s", SYM_AUDIO_DIR, FILENAME);
    if (g_file_test(link_path, G_FILE_TEST_EXISTS)) {
        g_print(RED_COLOR "[ERROR] Arquivo %s já existe\n" RESET_COLOR, FILENAME);
        g_free(link_path);
        g_free(clean_uri);
        return FALSE;
    }

    GFile *source_file = g_file_new_for_path(clean_uri);
    GFile *link_file = g_file_new_for_path(link_path);
    if (!g_file_make_symbolic_link(link_file, clean_uri, NULL, &error)) {
        g_print(RED_COLOR "[ERROR] Erro ao criar symlink: %s\n" RESET_COLOR, error->message);
        g_error_free(error);
        g_object_unref(source_file);
        g_object_unref(link_file);
        g_free(clean_uri);
        g_free(link_path);
        return FALSE;
    }
    g_object_unref(source_file);
    g_object_unref(link_file);

    GtkWidget *row = gtk_list_box_row_new();
    if (!row) {
        g_print("Erro: falha ao criar row\n");
        g_free(link_path);
        g_free(clean_uri);
        return FALSE;
    }

    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_hexpand(row_box, TRUE);
    gtk_widget_add_css_class(row_box, "music_row");

    GtkWidget *label = gtk_label_new(FILENAME);
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

    double duration = strcmp(ext, "mp3") == 0 ? get_duration(clean_uri) : get_duration_ogg(clean_uri);
    const char *duration_str = cast_double_to_string(duration);
    GtkWidget *duration_label = gtk_label_new(duration_str ? duration_str : "0.0");
    int offset = (85 - (strlen(duration_str) * 10)) >= 0 ? (85 - (strlen(duration_str) * 10)) : 0;
    offset = offset - 20;
    gtk_label_set_xalign(GTK_LABEL(duration_label), 1.0);
    gtk_widget_add_css_class(duration_label, "music_name_label");
    gtk_widget_set_margin_end(duration_label, offset);
    gtk_box_append(GTK_BOX(row_box), duration_label);

    WidgetsData *widgets_data = user_data;

    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
    gtk_list_box_insert(GTK_LIST_BOX(widgets_data->list_box), row, -1);

    g_object_set_data(G_OBJECT(row), "music_name", g_strdup(clean_uri));
    g_object_set_data(G_OBJECT(row), "music_duration", cast_simple_double_to_string(duration));

    g_free(link_path);
    g_free(clean_uri);

    return TRUE;
}
