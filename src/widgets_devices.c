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

void setup_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    GtkWidget *label = gtk_label_new(NULL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END); 
    gtk_label_set_single_line_mode(GTK_LABEL(label), TRUE); 
    gtk_list_item_set_child(list_item, label);
}


void bind_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    GtkWidget *label = gtk_list_item_get_child(list_item);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    GObject *item = gtk_list_item_get_item(list_item);

    if (GTK_IS_STRING_OBJECT(item)) {
        const char *text = gtk_string_object_get_string(GTK_STRING_OBJECT(item));
        gtk_label_set_text(GTK_LABEL(label), text);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "(Erro)");
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

    gtk_box_append(GTK_BOX(grid), virtual_box_audio_sink);
    gtk_box_append(GTK_BOX(grid), virtual_box_audio_source);
    
    gtk_widget_set_visible(grid, TRUE);
    gtk_widget_set_visible(virtual_box_audio_sink, TRUE);
    gtk_widget_set_visible(virtual_box_audio_source, TRUE);
}

void construct_widget(GtkWidget *button) {
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
    for (size_t i = 0; sinks[i] != NULL; i++) {
        if (sinks[i]->node_description != NULL && strlen(sinks[i]->node_description) != 0) {
            sink_count++;
        }
    }
    size_t source_count = 0;
    for (size_t i = 0; sources[i] != NULL; i++) {
        if (sources[i]->node_description != NULL && strlen(sources[i]->node_description) != 0) {
            source_count++;
        }
    }

    if (sink_count == 0 && source_count == 0) {
        printf("Nenhum sink ou source encontrado\n");
        free(sinks);
        free(sources);
        return;
    }

    struct audio_devices *audio_devices = malloc(sizeof(struct audio_devices));
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
    gtk_window_set_default_size(GTK_WINDOW(new_window), 400, 400);
    gtk_widget_add_css_class(GTK_WIDGET(new_window), "new_window_devices_class");
    GtkAllocation allocation = {0, 0, 400, 400};
    
    gtk_widget_size_allocate(GTK_WIDGET(new_window), &allocation, 0);

    if(allocation.width > 800) {
        printf("O tamanho da janela é muito grande\n");
    }

    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(GTK_WIDGET(new_window), controller);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); 
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER); 
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER); 
    gtk_window_set_child(GTK_WINDOW(new_window), box);

    GtkStringList *virtual_model_audio_sink = gtk_string_list_new(NULL);
    GtkStringList *virtual_model_audio_source = gtk_string_list_new(NULL);

    for (size_t i = 0; i < sink_count; i++) {
        gtk_string_list_append(virtual_model_audio_sink, audio_devices->sink[i]->node_description);
    }
    for (size_t i = 0; i < source_count; i++) {
        gtk_string_list_append(virtual_model_audio_source, audio_devices->source[i]->node_description);
    }

    GtkWidget *virtual_dropdown_audio_sink = gtk_drop_down_new(G_LIST_MODEL(virtual_model_audio_sink), NULL);
    GtkWidget *virtual_dropdown_audio_sources = gtk_drop_down_new(G_LIST_MODEL(virtual_model_audio_source), NULL);

    gtk_widget_set_halign(virtual_dropdown_audio_sink, GTK_ALIGN_CENTER); 
    gtk_widget_set_hexpand(virtual_dropdown_audio_sink, FALSE); 
    gtk_widget_set_halign(virtual_dropdown_audio_sources, GTK_ALIGN_CENTER); 
    gtk_widget_set_hexpand(virtual_dropdown_audio_sources, FALSE); 

    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), audio_devices);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_cb), audio_devices);

    gtk_drop_down_set_factory(GTK_DROP_DOWN(virtual_dropdown_audio_sink), factory);
    gtk_drop_down_set_factory(GTK_DROP_DOWN(virtual_dropdown_audio_sources), factory);

    GtkWidget *audio_sink_title = gtk_label_new("Audio Sink");
    GtkWidget *audio_source_title = gtk_label_new("Audio Source");

    gtk_box_append(GTK_BOX(box), audio_sink_title);
    gtk_box_append(GTK_BOX(box), virtual_dropdown_audio_sink);
    gtk_box_append(GTK_BOX(box), audio_source_title);
    gtk_box_append(GTK_BOX(box), virtual_dropdown_audio_sources);

    g_signal_connect(new_window, "destroy", G_CALLBACK(on_window_destroy), audio_devices);
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_press), GTK_WINDOW(new_window));

    gtk_widget_set_visible(new_window, TRUE);
}


