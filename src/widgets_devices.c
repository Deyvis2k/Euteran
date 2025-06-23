#include "widgets_devices.h"
#include "e_logs.h"
#include "audio_devices.h"
#include "adwaita.h"

static gboolean opened_window = FALSE;

static void on_window_destroy(GtkWidget *window, gpointer user_data) {
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

    opened_window = FALSE;
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
    struct audio_device **sinks = get_audio_devices(COMMAND_VIRTUAL_AUDIO_SINK);
    if (!sinks) {
        log_error("Erro: get_audio_devices retornou NULL");
        return;
    }
    struct audio_device **sources = get_audio_devices(COMMAND_VIRTUAL_AUDIO_SOURCE);
    if (!sources) {
        log_error("Erro: get_audio_devices retornou NULL");
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
        log_error("Erro: falha ao alocar audio_devices");
        free(sinks);
        free(sources);
        return;
    }

    audio_devices->sink = sink_count > 0 ? sinks : NULL;
    audio_devices->sink_count = sink_count;
    audio_devices->source = source_count > 0 ? sources : NULL;
    audio_devices->source_count = source_count;

    if(sink_count == 0 && source_count == 0) {
        log_error("Nenhum sink ou source encontrado");
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

void construct_widget(GtkWidget *button, gpointer user_data) {
    GtkWidget *window_parent = (GtkWidget *)user_data;
    if(!GTK_IS_WIDGET(window_parent) || window_parent == NULL) {
        log_error("Erro: window nao eh um GtkWidget"); 
        return;
    }
    if(opened_window) {
        log_warning("Janela ja aberta");
        return;
    }
    opened_window = TRUE;
    
    GtkWidget *menu_button_object = g_object_get_data(
    G_OBJECT(window_parent), "menu_button_popover");

    if(menu_button_object)
        gtk_menu_button_popdown(GTK_MENU_BUTTON(menu_button_object));


    struct audio_device **sinks = get_audio_devices(COMMAND_AUDIO_SINK);
    if (!sinks) {
        log_error("Erro: get_audio_devices retornou NULL");
        free(sinks);
        return;
    }
    struct audio_device **sources = get_audio_devices(COMMAND_AUDIO_SOURCE);
    if (!sources) {
        printf("Erro: get_audio_devices retornou NULL\n");
        log_error("Erro: get_audio_devices retornou NULL");
        free(sinks);
        free(sources);
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
        log_warning("Nenhum sink ou source encontrado");
        free(sinks);
        free(sources);
        return;
    }
    
    struct audio_devices *audio_devices = malloc(sizeof(struct audio_devices));
    if (!audio_devices) {
        log_error("Erro: falha ao alocar audio_devices");
        free(sinks);
        free(sources);
        free(audio_devices);
        return;
    }
    audio_devices->sink = sinks;
    audio_devices->sink_count = sink_count;
    audio_devices->source = sources;
    audio_devices->source_count = source_count;
    g_object_set_data(G_OBJECT(button), "audio_devices", audio_devices);
    
    GtkWidget *new_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(new_window), "Euteran-Top");
    gtk_window_set_default_size(GTK_WINDOW(new_window), 400, 400);
    gtk_widget_add_css_class(GTK_WIDGET(new_window), "new_window_devices_class");
    
    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(GTK_WIDGET(new_window), controller);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_FILL);
    gtk_widget_set_valign(box, GTK_ALIGN_START);
    gtk_window_set_child(GTK_WINDOW(new_window), box);
    
    GtkStringList *virtual_model_audio_sink = gtk_string_list_new(NULL);
    GtkStringList *virtual_model_audio_source = gtk_string_list_new(NULL);
    
    for (size_t i = 0; sinks[i] != NULL; i++) {
        if (sinks[i]->node_description != NULL && strlen(sinks[i]->node_description) != 0) {
            gtk_string_list_append(virtual_model_audio_sink, sinks[i]->node_description);
        }
    }
    
    for (size_t i = 0; sources[i] != NULL; i++) {
        if (sources[i]->node_description != NULL && strlen(sources[i]->node_description) != 0) {
            gtk_string_list_append(virtual_model_audio_source, sources[i]->node_description);
        }
    }
    
    if (sink_count > 0) {
        GtkWidget *sink_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_set_margin_bottom(sink_section, 16);
        
        GtkWidget *audio_sink_label = gtk_label_new("Audio Sink");
        gtk_widget_set_halign(audio_sink_label, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(sink_section), audio_sink_label);
        
        GtkWidget *sink_dropdown = gtk_drop_down_new(G_LIST_MODEL(virtual_model_audio_sink), NULL);
        gtk_widget_set_hexpand(sink_dropdown, TRUE);
        gtk_box_append(GTK_BOX(sink_section), sink_dropdown);
        
        gtk_box_append(GTK_BOX(box), sink_section);
    }
    
    if (source_count > 0) {
        GtkWidget *source_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        
        GtkWidget *audio_source_label = gtk_label_new("Audio Source");
        gtk_widget_set_halign(audio_source_label, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(source_section), audio_source_label);
        
        GtkWidget *source_dropdown = gtk_drop_down_new(G_LIST_MODEL(virtual_model_audio_source), NULL);
        gtk_widget_set_hexpand(source_dropdown, TRUE);
        gtk_box_append(GTK_BOX(source_section), source_dropdown);
        
        gtk_box_append(GTK_BOX(box), source_section);
    }
    
    GtkWidget *clamp = adw_clamp_new();
    adw_clamp_set_maximum_size(ADW_CLAMP(clamp), 600); 
    gtk_widget_set_margin_top(clamp, 16);

    GtkWidget *label = gtk_label_new(
        "Click on confirm to synchronize the sink and source.\n"
        "Note that this will create a new sink and source with the same name."
    );
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(GTK_WIDGET(label), "label_devices_class");
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);

    adw_clamp_set_child(ADW_CLAMP(clamp), label);
    gtk_box_append(GTK_BOX(box), clamp);
    
    
    GtkWidget *confirm_button = gtk_button_new_with_label("Confirm");
    gtk_widget_set_halign(confirm_button, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(confirm_button, 16);
    gtk_box_append(GTK_BOX(box), confirm_button);
    
    g_signal_connect(new_window, "destroy", G_CALLBACK(on_window_destroy), audio_devices);
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_press), GTK_WINDOW(new_window));
    
    gtk_widget_set_visible(new_window, TRUE);
}
