#include "eut_subwindow.h"
#include "adwaita.h"
#include "eut_logs.h"
#include "eut_subwindowimpl.h"
#include "eut_audiodevices.h"
#include "glib.h"
#include "gtk/gtk.h"
#include <pwd.h>

#define initialize_hashs_ptr(ptr, item, index) \
    ((void **)(ptr))[index] = (void *)(item); \




struct _EutSubwindow {
    AdwWindow parent_instance;

    gboolean opened_window;

    GtkDropDown *sink_dropdown;
    GtkDropDown *source_dropdown;
    GtkDropDown *theme_dropdown;
    GtkDropDown *icon_dropdown;
    GtkStringList *sink_list;
    GtkStringList *source_list;

    GtkStringList *theme_name_list;
    GtkStringList *theme_icon_list;
    GtkButton *confirm_button;

    GtkSettings *main_object_settings;


    

    GHashTable* themes_hash_table;
    GHashTable* icons_hash_table;

    GHashTable* hashs[MAX_HASH_ITEMS];
    GtkStringList* dropdown_lists[MAX_HASH_ITEMS];

    struct audio_devices *audio_devices;

    struct {
        int width;
        int height;
    } WinAllocatedSize;
};

G_DEFINE_TYPE(EutSubwindow, eut_subwindow, ADW_TYPE_WINDOW);


void
on_button_clicked
(
    EutSubwindow *self
)
{
    if(EUT_IS_SUBWINDOW(self)) {
        log_info("I'm subwindow");
    }
    else {
        log_info("I'm not subwindow");
    }
}

static void 
eut_subwindow_dispose(GObject *object) 
{
    EutSubwindow *self = EUT_SUBWINDOW(object);

    log_warning("Cleaning Euteran Subwindow");

    if (self->themes_hash_table) {
        g_hash_table_destroy(self->themes_hash_table);
        self->themes_hash_table = nullptr;
    }

   if(self->audio_devices) {
        if(self->audio_devices->audio_device_sink) g_list_free(self->audio_devices->audio_device_sink);
        if(self->audio_devices->audio_device_source) g_list_free(self->audio_devices->audio_device_source);
        free(self->audio_devices);
        self->audio_devices = nullptr;
    }

    G_OBJECT_CLASS(eut_subwindow_parent_class)->dispose(object);
}

static void 
eut_subwindow_class_init(EutSubwindowClass *klass) 
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    object_class->dispose = eut_subwindow_dispose;

    gtk_widget_class_set_template_from_resource(widget_class, "/org/euteran/euteran_audio_devices.ui");

    gtk_widget_class_bind_template_child(widget_class, EutSubwindow, sink_dropdown);
    gtk_widget_class_bind_template_child(widget_class, EutSubwindow, source_dropdown);
    gtk_widget_class_bind_template_child(widget_class, EutSubwindow, theme_dropdown);
    gtk_widget_class_bind_template_child(widget_class, EutSubwindow, sink_list);
    gtk_widget_class_bind_template_child(widget_class, EutSubwindow, source_list);
    gtk_widget_class_bind_template_child(widget_class, EutSubwindow, theme_name_list);
    gtk_widget_class_bind_template_child(widget_class, EutSubwindow, theme_icon_list);
    gtk_widget_class_bind_template_child(widget_class, EutSubwindow, icon_dropdown);
    gtk_widget_class_bind_template_child(widget_class, EutSubwindow, confirm_button);
    
    gtk_widget_class_bind_template_callback(widget_class, on_button_clicked);
    gtk_widget_class_bind_template_callback(widget_class, on_dropdown_theme_selected);
    gtk_widget_class_bind_template_callback(widget_class, on_dropdown_icon_selected);

}

static void 
eut_subwindow_init(EutSubwindow *self) 
{
    gtk_widget_init_template(GTK_WIDGET(self));
    self->opened_window = FALSE;
    self->WinAllocatedSize.width = 600;
    self->WinAllocatedSize.height = 400;
    self->themes_hash_table = nullptr;
    self->icons_hash_table = nullptr;
    self->audio_devices = nullptr;

    initialize_hashs_ptr(self->hashs, self->themes_hash_table, THEME_HASH);
    initialize_hashs_ptr(self->hashs, self->icons_hash_table, ICON_HASH);

    initialize_hashs_ptr(self->dropdown_lists, self->theme_name_list, THEME_HASH);
    initialize_hashs_ptr(self->dropdown_lists, self->theme_icon_list, ICON_HASH);

}

EutSubwindow *
eut_subwindow_new() 
{
    return g_object_new(TYPE_EUT_SUBWINDOW, nullptr);   
}

void 
eut_subwindow_fill_stringlist
(
    EutSubwindow *self
)
{
    for(GList* node = self->audio_devices->audio_device_sink; node != nullptr; node = node->next) {
        struct audio_device* audio_device = (struct audio_device*)node->data;
        if (audio_device->node_description != nullptr && strlen(audio_device->node_description) != 0) {
            if (g_utf8_validate(audio_device->node_description, -1, nullptr)) {
                gtk_string_list_append(self->sink_list, audio_device->node_description);
            } 
            
        } 
    }
    for(GList* node = self->audio_devices->audio_device_source; node != nullptr; node = node->next) {
        struct audio_device* audio_device = (struct audio_device*)node->data;
        if (audio_device->node_description != nullptr && strlen(audio_device->node_description) != 0
            && g_utf8_validate(audio_device->node_description, -1, nullptr)
        ) {
            gtk_string_list_append(self->source_list, audio_device->node_description);
        } else {
            log_warning("Device sem descricao");
        }
    }
}

void 
eut_subwindow_set_drop_down
(
    EutSubwindow *self,
    GtkDropDown *drop_down,
    enum type_audio_device type
) {
    g_return_if_fail(EUT_IS_SUBWINDOW(self) && drop_down != nullptr);

    if (type == AUDIO_SOURCE) {
        self->source_dropdown = drop_down;
    } else {
        self->sink_dropdown = drop_down;
    }
}

GtkWindow 
*eut_subwindow_get_window
(
    EutSubwindow *self
) {
    g_return_val_if_fail(EUT_IS_SUBWINDOW(self), nullptr);

    return GTK_WINDOW(self);
}

GtkStringList *
eut_subwindow_get_stringlist(
    EutSubwindow             *self, 
    enum type_audio_device   type
)
{
    g_return_val_if_fail(EUT_IS_SUBWINDOW(self), nullptr);

    return type == AUDIO_SOURCE ? self->source_list : self->sink_list;
}


void 
eut_subwindow_init_audio_devices(EutSubwindow *self)
{
    g_return_if_fail(EUT_IS_SUBWINDOW(self));

    if(self->audio_devices != nullptr) return;

    self->audio_devices = g_new0(struct audio_devices, 1);
    self->audio_devices->audio_device_sink = get_audio_devices(COMMAND_AUDIO_SINK);
    self->audio_devices->audio_device_source = get_audio_devices(COMMAND_AUDIO_SOURCE);

    eut_subwindow_fill_stringlist(self);
}

void 
eut_subwindow_set_window_visibility
(
    EutSubwindow *self,
    gboolean visible
) {
    g_return_if_fail(EUT_IS_SUBWINDOW(self));

    gtk_widget_set_visible(GTK_WIDGET(self), visible);
}

void 
eut_subwindow_set_allocated_window_size
(
    EutSubwindow *self
)
{
    g_return_if_fail(EUT_IS_SUBWINDOW(self));

    int width, height;
    gtk_window_get_default_size(GTK_WINDOW(self), &width, &height);
    self->WinAllocatedSize.width = width;
    self->WinAllocatedSize.height = height;
}

void 
eut_subwindow_set_window_size
(
    EutSubwindow *self
)
{
    g_return_if_fail(EUT_IS_SUBWINDOW(self));

    gtk_window_set_default_size(GTK_WINDOW(self), self->WinAllocatedSize.width, self->WinAllocatedSize.height);
}


GtkDropDown *
eut_subwindow_get_dropdown
(
    EutSubwindow *self,
    enum type_dropdown type
) {
    g_return_val_if_fail(EUT_IS_SUBWINDOW(self), nullptr);

    switch (type) {
        case DROPDOWN_SOURCE:
            return self->source_dropdown;
        case DROPDOWN_SINK:
            return self->sink_dropdown;
        case DROPDOWN_THEME:
            return self->theme_dropdown;
        case DROPDOWN_ICON:
            return self->icon_dropdown;
        default:
            return nullptr;
    }
}

static gboolean 
is_subdirectory_empty
(
    const gchar* subdirectory_path
)
{
    DIR *dir = opendir(subdirectory_path);
    if(!dir){
        return TRUE;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != nullptr){
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
            closedir(dir);
            return FALSE;
        }
    }
    closedir(dir);
    return TRUE;
}

static const gchar *
get_home_name
(
    void
)
{
    const gchar *home = g_get_home_dir();
    if(home == nullptr) return nullptr;
    return home;
}

static char ** 
open_dir_and_append_to_hash
(
    enum hash_table_type type,
    size_t*   theme_count
)
{
    const char *format_paths[][3] = {
        {"/usr/share/icons/", "%s/.local/share/icons/", "%s/.icons/"},
        {"/usr/share/themes/", "%s/.local/share/themes/", "%s/.themes/"},
    };

    char all_posible_paths[MAX_HASH_ITEMS][3][1024];

    const gchar *home_name = get_home_name();
    if(home_name == nullptr) return nullptr;

    for(int i = 0; i < MAX_HASH_ITEMS; i++){
        strcpy(all_posible_paths[i][0], format_paths[i][0]);
        g_snprintf(all_posible_paths[i][1], sizeof(all_posible_paths[i][1]), format_paths[i][1], home_name);
        g_snprintf(all_posible_paths[i][2], sizeof(all_posible_paths[i][2]), format_paths[i][2], home_name);
    }

    const char (*paths)[1024] = all_posible_paths[type];

    size_t MAX_ITEMS = 10;
    char **system_themes = calloc(MAX_ITEMS, sizeof(char*));
    gchar subdir[1024] = {0};
    for(const char *path = *paths; path != nullptr && path[0] != '\0'; path = *(++paths)) {
        DIR *dir = opendir(path);
        if(dir == nullptr) continue;
        
        struct dirent *entry;
        while((entry = readdir(dir)) != nullptr) {
            if(*theme_count == MAX_ITEMS){
                MAX_ITEMS *= 2;
                char **temp = realloc(system_themes, sizeof(char *) * MAX_ITEMS);
                if(temp != nullptr){
                    system_themes = temp;
                } 
            }
            if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
                g_snprintf(subdir, sizeof(subdir), "%s%s", path, entry->d_name);
                if (is_subdirectory_empty(subdir)) {
                    log_info("Skipping empty subdirectory: %s", subdir);
                    continue;
                }


                system_themes[*theme_count] = strdup(entry->d_name);
                (*theme_count)++;
            }
        }

        closedir(dir);
    }
    return system_themes;
}



static void 
append_to_string_list
(
    GtkStringList *model_to_append,
    GList *sorted_list,
    enum hash_table_type type
)
{
    switch (type) {
        case ICON_HASH:
            GtkIconTheme *icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
            gchar *icon_name = gtk_icon_theme_get_theme_name(icon_theme);
            gboolean found = FALSE;
            for (GList *node = sorted_list; node != nullptr; node = node->next) {
                if(icon_name != nullptr && found == FALSE) {
                    gtk_string_list_append(model_to_append, icon_name);
                    found = TRUE;
                }
                if(g_strcmp0(icon_name, node->data) != 0) gtk_string_list_append(model_to_append, node->data);
            }
            break;
        case THEME_HASH:
            GtkSettings *settings = gtk_settings_get_default();
            gchar *gtk_theme_name = nullptr;
            gboolean found_ = FALSE;
            g_object_get(settings, "gtk-theme-name", &gtk_theme_name, nullptr);
            for (GList *node = sorted_list; node != nullptr; node = node->next) {
                if(gtk_theme_name != nullptr && found_ == FALSE) {
                    gtk_string_list_append(model_to_append, gtk_theme_name);
                    found_ = TRUE;
                }
                if(g_strcmp0(gtk_theme_name, node->data) != 0) gtk_string_list_append(model_to_append, node->data);
            }
            break;
        default:
            break;
    }
}

void 
eut_subwindow_fill_hashtable
(
    EutSubwindow *self,
    enum hash_table_type type
)
{
    g_return_if_fail(EUT_IS_SUBWINDOW(self) && (type < MAX_HASH_ITEMS && type >= 0));
    GHashTable *hash_to_implement = self->hashs[type];
    if(hash_to_implement != nullptr) {
        log_error("already implemented %d", type);
    }

    GtkStringList *model_to_append = self->dropdown_lists[type];

    GHashTableIter iter;
    GList *sorted_list = nullptr;
    hash_to_implement = g_hash_table_new_full(g_str_hash, g_str_equal, free, nullptr);

    size_t theme_count = 0;
    char **system_themes;

    system_themes = open_dir_and_append_to_hash(type, &theme_count);

    for(size_t i = 0; i < theme_count; i++) {
        g_hash_table_add(hash_to_implement, g_strdup(system_themes[i]));
        free(system_themes[i]);
    }
    

    free(system_themes);

    char *theme;
    g_hash_table_iter_init(&iter, hash_to_implement);
    while (g_hash_table_iter_next(&iter, (gpointer *)&theme, nullptr)) {
        sorted_list = g_list_insert_sorted(sorted_list, theme, (GCompareFunc)strcmp);
    }

    append_to_string_list(model_to_append, sorted_list, type);


    g_list_free(sorted_list);
}

gboolean
eut_subwindow_get_opened_window(
    EutSubwindow *self
) 
{
    g_return_val_if_fail(EUT_IS_SUBWINDOW(self), FALSE);

    return self->opened_window;    
}

void 
eut_subwindow_set_opened_window
(
    EutSubwindow *self,
    gboolean     opened
) {
    g_return_if_fail(EUT_IS_SUBWINDOW(self));

    self->opened_window = opened;
}

void eut_subwindow_set_settings
(
    EutSubwindow *self, 
    GtkSettings *settings
)
{
    g_return_if_fail(EUT_IS_SUBWINDOW(self) && settings != nullptr);

    self->main_object_settings = settings;
}

GtkSettings* eut_subwindow_get_main_object_settings
(
    EutSubwindow *self
)
{
    g_return_val_if_fail(EUT_IS_SUBWINDOW(self), nullptr);
    return self->main_object_settings;
}
