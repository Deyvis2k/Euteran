#include "widgets_devices.h"
#include "gtk/gtk.h"
#include "utils.h"


static void on_window_destroy(GtkWidget *window, gpointer user_data) {
    g_print("deleting widgets_device\n");
    struct audio_devices *audio_devices = (struct audio_devices *)user_data;
    if(audio_devices) {
        if (audio_devices->sink) {
            for (size_t i = 0; i < audio_devices->sink_count; i++) {
                if (audio_devices->sink[i]) {
                    free(audio_devices->sink[i]->node_description);
                    free(audio_devices->sink[i]->node_name);
                    free(audio_devices->sink[i]);
                }
            }
            free(audio_devices->sink);
        }
        
        if (audio_devices->source) {
            for (size_t i = 0; i < audio_devices->source_count; i++) {
                if (audio_devices->source[i]) {
                    free(audio_devices->source[i]->node_description);
                    free(audio_devices->source[i]->node_name);
                    free(audio_devices->source[i]);
                }
            }
            free(audio_devices->source);
        }

        free(audio_devices);
        audio_devices = NULL;
    }
}

gboolean on_key_press(GtkEventControllerKey *controller, guint keyval,
    guint keycode,
    GdkModifierType state,
    gpointer user_data) {
    GtkWindow *window = GTK_WINDOW(user_data);
    if (keyval == GDK_KEY_Escape) {
        gtk_window_close(window);
        return TRUE;
    }
    return FALSE;
}

void construct_virtual_audio_widget(GtkWidget *grid) {
    printf("Constructing virtual audio widget\n");
    struct audio_device **sinks = get_audio_devices(COMMAND_VIRTUAL_AUDIO_SINK);
    if (!sinks) {
        printf("Erro: get_audio_devices retornou NULL\n");
        return;
    }
    struct audio_device **sources = get_audio_devices(COMMAND_VIRTUAL_AUDIO_SOURCE);
    if (!sources) {
        printf("Erro: get_audio_devices retornou NULL\n");
        return;
    }

    size_t sink_count = 0;
    for(size_t i = 0; sinks[i] != NULL; i++) {
        sink_count++;
        if (sinks[i]->node_description != NULL && strlen(sinks[i]->node_description) != 0) {
            sink_count++;
        }
    }

    size_t source_count = 0;
    for(size_t i = 0; sources[i] != NULL; i++) {
        source_count++;
        if (sources[i]->node_description != NULL && strlen(sources[i]->node_description) != 0) {
            source_count++;
        }
    }

    struct audio_devices* audio_devices = malloc(sizeof(struct audio_devices));
    if (!audio_devices) {
        printf("Erro: falha ao alocar audio_devices\n");
        free(sinks);
        free(sources);
        return;
    }

    audio_devices->sink = sink_count > 0 ? sinks : NULL;
    audio_devices->sink_count = sink_count;
    audio_devices->source = source_count > 0 ? sources : NULL;
    audio_devices->source_count = source_count;

    if(sink_count == 0 && source_count == 0) {
        printf("Nenhum sink ou source encontrado\n");
        free(audio_devices);
        free(sinks);
        free(sources);
        return;
    }

    GtkWidget *virtual_box_audio_sink = gtk_list_box_new();
    GtkWidget *virtual_box_audio_source = gtk_list_box_new();

    gtk_widget_add_css_class(virtual_box_audio_sink, "list_box_sinks_class");
    gtk_widget_add_css_class(virtual_box_audio_source, "list_box_sources_class");

    for (size_t i = 0; i < sink_count; i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(sinks[i]->node_description);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_insert(GTK_LIST_BOX(virtual_box_audio_sink), row, i);
    }

    for (size_t i = 0; i < source_count; i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(sources[i]->node_description);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_insert(GTK_LIST_BOX(virtual_box_audio_source), row, i);
    }

    g_object_set_data(G_OBJECT(grid), "virtual_audio_devices", audio_devices);

    gtk_grid_attach(GTK_GRID(grid), virtual_box_audio_sink, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), virtual_box_audio_source, 0, 3, 1, 1);
    
    gtk_widget_set_visible(grid, TRUE);
    gtk_widget_set_visible(virtual_box_audio_sink, TRUE);
    gtk_widget_set_visible(virtual_box_audio_source, TRUE);
}

void construct_widget(GtkWidget *button){
    struct audio_device **sinks = get_audio_devices(COMMAND_AUDIO_SINK);
    if (!sinks) {
        printf("Erro: get_audio_devices retornou NULL\n");
        return;
    }
    struct audio_device **sources = get_audio_devices(COMMAND_AUDIO_SOURCE);
    if (!sources) {
        printf("Erro: get_audio_devices retornou NULL\n");
        return;
    }
    size_t sink_count = 0;
    for(size_t i = 0; sinks[i] != NULL; i++) {
        if(sinks[i]->node_description != NULL && strlen(sinks[i]->node_description) != 0) {
            sink_count++;
        }
    }
    size_t source_count = 0;
    for(size_t i = 0; sources[i] != NULL; i++) {
        if(sources[i]->node_description != NULL && strlen(sources[i]->node_description) != 0) {
           source_count++;
        }
    }

    if(sink_count == 0 && source_count == 0) {
        printf("Nenhum sink ou source encontrado\n");
        free(sinks);
        free(sources);
        return;
    }

    struct audio_devices* audio_devices = malloc(sizeof(struct audio_devices));
    if (!audio_devices) {
        printf("Erro: falha ao alocar audio_devices\n");
        free(sinks);
        free(sources);
        return;
    }
    audio_devices->sink = sinks;
    audio_devices->sink_count = sink_count;
    audio_devices->source = sources;
    audio_devices->source_count = source_count;

    g_object_set_data(G_OBJECT(button), "audio_devices", audio_devices);

    GtkWidget *new_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(new_window), "Background");
    gtk_window_set_default_size(GTK_WINDOW(new_window), 450, 400);
    gtk_window_set_resizable(GTK_WINDOW(new_window), FALSE);
    gtk_widget_set_hexpand(GTK_WIDGET(new_window), FALSE);

    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(GTK_WIDGET(new_window), controller);

    GtkWidget *main_grid = gtk_grid_new();
    gtk_widget_add_css_class(GTK_WIDGET(new_window), "main_grid_devices_class");
    gtk_widget_set_size_request(main_grid, 450, 400);
    gtk_window_set_child(GTK_WINDOW(new_window), main_grid);
    
    GtkWidget *vertical_box_sinks = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_widget_set_size_request(vertical_box_sinks, 350, -1);
    gtk_widget_set_halign(GTK_WIDGET(vertical_box_sinks), GTK_ALIGN_CENTER);
    gtk_widget_set_valign(GTK_WIDGET(vertical_box_sinks), GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(GTK_WIDGET(vertical_box_sinks), FALSE);
    GtkWidget *vertical_box_sources = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(vertical_box_sinks, 350, -1);
    gtk_widget_set_halign(GTK_WIDGET(vertical_box_sources), GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(GTK_WIDGET(vertical_box_sources), FALSE);
    gtk_widget_set_valign(GTK_WIDGET(vertical_box_sources), GTK_ALIGN_CENTER);
    

    GtkWidget *list_box_sinks = gtk_list_box_new();
    gtk_widget_set_halign(GTK_WIDGET(list_box_sinks), GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(GTK_WIDGET(list_box_sinks), FALSE);
    GtkWidget *list_box_sources = gtk_list_box_new();
    gtk_widget_set_halign(GTK_WIDGET(list_box_sources), GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(GTK_WIDGET(list_box_sources), FALSE);

    GtkWidget *container_real_devices = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(GTK_WIDGET(container_real_devices), "container_real_devices_class");
    gtk_widget_set_size_request(container_real_devices, 400, -1);
    gtk_widget_set_halign(GTK_WIDGET(container_real_devices), GTK_ALIGN_CENTER);
    gtk_widget_set_valign(GTK_WIDGET(container_real_devices), GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(GTK_WIDGET(container_real_devices), FALSE);

    gtk_widget_add_css_class(GTK_WIDGET(list_box_sinks), "list_box_sinks_class");
    gtk_widget_add_css_class(GTK_WIDGET(list_box_sources), "list_box_sources_class");
    
    g_assert(GTK_IS_BOX(vertical_box_sinks) && GTK_IS_BOX(vertical_box_sources));
    
    for(size_t i = 0; i < sink_count; i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(sinks[i]->node_description);

        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(list_box_sinks), row);
    }

    for(size_t i = 0; i < source_count; i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(sources[i]->node_description);

        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(list_box_sources), row);
    }
    gtk_box_append(GTK_BOX(vertical_box_sinks), list_box_sinks);
    gtk_box_append(GTK_BOX(vertical_box_sources), list_box_sources);
    gtk_box_append(GTK_BOX(container_real_devices), vertical_box_sinks);
    gtk_box_append(GTK_BOX(container_real_devices), vertical_box_sources);
    gtk_grid_attach(GTK_GRID(main_grid), container_real_devices, 0, 0, 1, 1);

    construct_virtual_audio_widget(main_grid);
    
    g_signal_connect(new_window, "destroy", G_CALLBACK(on_window_destroy), audio_devices);
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_press), GTK_WINDOW(new_window));

    gtk_widget_set_visible(new_window, TRUE);
}   

