#include "widgets_devices.h"
#include "e_logs.h"
#include "audio_devices.h"
#include "euteran_main_object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "gtk/gtkshortcut.h"
#include "utils.h"

static gboolean opened_window = FALSE;
static int last_width, last_height = 0;

void 
on_setup_list_item(
    GtkSignalListItemFactory    *factory,
    GtkListItem                 *list_item,
    gpointer                    user_data
)
{
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_list_item_set_child(GTK_LIST_ITEM(list_item), label);
}

void on_bind_list_item(GtkSignalListItemFactory *factory,
                       GtkListItem *list_item,
                       gpointer user_data)
{
    GtkLabel *label = GTK_LABEL(gtk_list_item_get_child(list_item));
    if (!label) {
        log_error("Erro ao obter label");
        return;
    }

    const char *str = gtk_list_item_get_item(list_item);
    GtkDropDown *dropdown = GTK_DROP_DOWN(user_data); 
    if (str && g_utf8_validate(str, -1, NULL)) {
        gtk_label_set_text(label, str);
    } 

    if (dropdown) {
        GtkStringList *model = GTK_STRING_LIST(gtk_drop_down_get_model(dropdown));
        if (model) {
            int position = gtk_list_item_get_position(list_item);
            const char *original = gtk_string_list_get_string(model, position);
            if (original && g_utf8_validate(original, -1, NULL)) {
                gtk_label_set_text(label, original);
            }
        }
    }
}

static void 
free_audio_device(gpointer data) {
    struct audio_device* device = (struct audio_device*)data;
    if (device) {
        g_free(device->node_name);
        g_free(device->node_description);
        free(device);
    }
}

static void 
on_window_destroy(
    GtkWidget   *window, 
    gpointer    user_data
) 
{
    struct audio_devices *audio_devices = (struct audio_devices *)user_data;
    if (audio_devices) {
        if(audio_devices->audio_device_sink)
            g_list_free_full(audio_devices->audio_device_sink, free_audio_device);
        if(audio_devices->audio_device_source)
            g_list_free_full(audio_devices->audio_device_source, free_audio_device);
        free(audio_devices);
    }

    gtk_window_get_default_size(GTK_WINDOW(window), &last_width, &last_height);

    opened_window = FALSE;
}

gboolean on_key_press(
    GtkEventControllerKey *controller, 
    guint                 keyval,
    guint                 keycode,
    GdkModifierType       state,
    gpointer              user_data
) 
{
    GtkWindow *window = GTK_WINDOW(user_data);
    if (keyval == GDK_KEY_Escape) {
        gtk_window_close(window);
        return TRUE;
    }
    return FALSE;
}

void construct_widget(GtkWidget *button, gpointer user_data) {
    EuteranMainObject *wd = EUTERAN_MAIN_OBJECT(user_data);
    GtkWidget *window_parent = GTK_WIDGET(euteran_main_object_get_widget_at(wd, WINDOW_PARENT));
    if (!GTK_IS_WIDGET(window_parent) || window_parent == NULL) {
        log_error("Erro: window nao eh um GtkWidget");
        return;
    }
    if (opened_window) {
        log_warning("Janela ja aberta");
        return;
    }
    opened_window = TRUE;

    GtkWidget *menu_button_object = GTK_WIDGET(euteran_main_object_get_widget_at(wd, MENU_BUTTON));
    if (menu_button_object)
        gtk_menu_button_popdown(GTK_MENU_BUTTON(menu_button_object));
    
    struct audio_devices *audio_devices = g_new0(struct audio_devices, 1);
    audio_devices->audio_device_sink = get_audio_devices(COMMAND_AUDIO_SINK);
    audio_devices->audio_device_source = get_audio_devices(COMMAND_AUDIO_SOURCE);

    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css_provider, "Style/style.css");

    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                              GTK_STYLE_PROVIDER(css_provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);



    GtkBuilder *builder = gtk_builder_new_from_file("ux/euteran_audio_devices.ui");

    GtkWidget *new_window = BUILDER_GET(builder, "Euteran-audio_window");
    GtkWidget *box = BUILDER_GET(builder, "main_box");
    GtkWidget *sink_label = BUILDER_GET(builder, "sink_label");
    GtkWidget *source_label = BUILDER_GET(builder, "source_label");
    GtkWidget *confirm_button = BUILDER_GET(builder, "confirm_button");
    GtkStringList *virtual_model_audio_sink = GTK_STRING_LIST(gtk_builder_get_object(builder, "sink_list"));
    GtkStringList *virtual_model_audio_source = GTK_STRING_LIST(gtk_builder_get_object(builder, "source_list"));
    GtkDropDown *dropdown_sink = GTK_DROP_DOWN(gtk_builder_get_object(builder, "sink_dropdown"));
    GtkDropDown *dropdown_source = GTK_DROP_DOWN(gtk_builder_get_object(builder, "source_dropdown"));

    g_return_if_fail( 
        new_window                  != NULL &&
        box                         != NULL && 
        sink_label                  != NULL && 
        source_label                != NULL && 
        confirm_button              != NULL && 
        virtual_model_audio_sink    != NULL && 
        virtual_model_audio_source  != NULL && 
        dropdown_sink               != NULL && 
        dropdown_source             != NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(new_window), last_width, last_height);
    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(new_window, controller);

    for(GList* node = audio_devices->audio_device_sink; node != NULL; node = node->next) {
        struct audio_device* audio_device = (struct audio_device*)node->data;
        if (audio_device->node_description != NULL && strlen(audio_device->node_description) != 0) {
            if (g_utf8_validate(audio_device->node_description, -1, NULL)) {
                gtk_string_list_append(virtual_model_audio_sink, audio_device->node_description);
            } 
            
        } 
    }

    for(GList* node = audio_devices->audio_device_source; node != NULL; node = node->next) {
        struct audio_device* audio_device = (struct audio_device*)node->data;
        if (audio_device->node_description != NULL && strlen(audio_device->node_description) != 0
            && g_utf8_validate(audio_device->node_description, -1, NULL)
        ) {
            gtk_string_list_append(virtual_model_audio_source, audio_device->node_description);
        } else {
            log_warning("Device sem descricao");
        }
    }

    GtkListItemFactory *factory_sink = gtk_signal_list_item_factory_new();
    GtkListItemFactory *factory_source = gtk_signal_list_item_factory_new();

    g_signal_connect(factory_sink, "setup", G_CALLBACK(on_setup_list_item), dropdown_sink);
    g_signal_connect(factory_sink, "bind", G_CALLBACK(on_bind_list_item), dropdown_sink);

    g_signal_connect(factory_source, "setup", G_CALLBACK(on_setup_list_item), dropdown_source);
    g_signal_connect(factory_source, "bind", G_CALLBACK(on_bind_list_item), dropdown_source);

    gtk_drop_down_set_factory(dropdown_source, GTK_LIST_ITEM_FACTORY(factory_source));
    gtk_drop_down_set_factory(dropdown_sink, GTK_LIST_ITEM_FACTORY(factory_sink));

    g_signal_connect(new_window, "destroy", G_CALLBACK(on_window_destroy), audio_devices);
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_press), GTK_WINDOW(new_window));

    gtk_widget_set_visible(new_window, TRUE);
    g_object_unref(css_provider);
    g_object_unref(builder);
}
