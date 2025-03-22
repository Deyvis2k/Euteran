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

void create_music_list(const gchar *path, WidgetsData *widgets_data, GtkWidget *grid, PlayMusicFunc play_selected_music) {
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
        if (new_music_list.musics[i].name == NULL || new_music_list.musics[i].duration <= 0) {
            printf(RED_COLOR "[ERROR] Música inválida no índice %zu\n" RESET_COLOR, i);
            continue;
        }

        GtkWidget *row = gtk_list_box_row_new();
        if (!row) {
            g_print("Erro: falha ao criar row\n");
            continue;
        }

        GtkWidget *grid = gtk_grid_new();

        gtk_widget_set_halign(GTK_WIDGET(grid), GTK_ALIGN_CENTER);
        gtk_widget_set_size_request(GTK_WIDGET(grid), 650, 10);

        GtkWidget *label = gtk_label_new(new_music_list.musics[i].name);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0); 
        gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_CENTER); 
        gtk_widget_set_margin_end(GTK_WIDGET(label), 100);
        gtk_widget_set_hexpand(GTK_WIDGET(label), TRUE); //

        const char *duration_str = cast_double_to_string(new_music_list.musics[i].duration);
        GtkWidget *duration = gtk_label_new(duration_str ? duration_str : "0.0");
        gtk_label_set_xalign(GTK_LABEL(duration), 1.0); 
        gtk_widget_set_halign(GTK_WIDGET(duration), GTK_ALIGN_END); 
        gtk_widget_add_css_class(GTK_WIDGET(duration), "duration_class");
        gtk_widget_set_margin_end(GTK_WIDGET(duration), 40);

        gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1); 
        gtk_grid_attach(GTK_GRID(grid), duration, 1, 0, 1, 1);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), grid);
        gtk_list_box_insert(GTK_LIST_BOX(widgets_data->list_box), row, (gint)i);
        g_object_set_data(G_OBJECT(row), "music_name", new_music_list.musics[i].name);
        g_object_set_data(G_OBJECT(row), "music_duration", cast_simple_double_to_string((new_music_list.musics[i].duration)));
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
