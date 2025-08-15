#pragma once

#include "adwaita.h"
#include "glib.h"
#define COMMAND_AUDIO_SOURCE "pw-dump | grep -A 50 'Audio/Source' | grep -E 'object.id|node.name|node.description'"
#define COMMAND_AUDIO_SINK "pw-dump | grep -A 50 'Audio/Sink' | grep -E 'object.id|node.name|node.description'"

#define COMMAND_VIRTUAL_AUDIO_SINK "pw-dump | grep -A 10 'VirtualMicSink' | grep -E 'object.id|node.name|node.description'"
#define COMMAND_VIRTUAL_AUDIO_SOURCE "pw-dump | grep -A 10 'VirtualMicSource' | grep -E 'object.id|node.name|node.description'"

#include <gtk/gtk.h>

enum type_audio_device{
    AUDIO_SOURCE = 0,
    AUDIO_SINK = 1
};

enum type_dropdown{
    DROPDOWN_SOURCE = 0,
    DROPDOWN_SINK = 1,
    DROPDOWN_THEME = 2,
    DROPDOWN_ICON = 3
};

enum hash_table_type{
    ICON_HASH,
    THEME_HASH,
    MAX_HASH_ITEMS
};


G_BEGIN_DECLS

#define TYPE_EUT_SUBWINDOW (eut_subwindow_get_type())
G_DECLARE_FINAL_TYPE(EutSubwindow, eut_subwindow, EUT, SUBWINDOW, AdwWindow)

EutSubwindow *eut_subwindow_new();
GtkBuilder *eut_subwindow_get_builder(EutSubwindow *self);
GtkWindow *eut_subwindow_get_window(EutSubwindow *self);
GtkDropDown *eut_subwindow_get_dropdown(EutSubwindow *self, enum type_dropdown type);
struct audio_devices* eut_subwindow_get_audio_devices(EutSubwindow* self);
GtkStringList *eut_subwindow_get_stringlist(EutSubwindow* self, enum type_audio_device type);
gboolean eut_subwindow_get_opened_window(EutSubwindow* self);
GtkSettings* eut_subwindow_get_main_object_settings(EutSubwindow* self);

void eut_subwindow_fill_stringlist(EutSubwindow *self);
void eut_subwindow_init_audio_devices(EutSubwindow *self);
void eut_subwindow_set_window_visibility(EutSubwindow *self, gboolean visible);
void eut_subwindow_set_window_size(EutSubwindow* self);
void eut_subwindow_set_allocated_window_size(EutSubwindow* self);
void eut_subwindow_fill_hashtable(EutSubwindow* self, enum hash_table_type type);
void eut_subwindow_set_opened_window(EutSubwindow* self, gboolean opened);
void eut_subwindow_set_settings(EutSubwindow* self, GtkSettings* settings);

G_END_DECLS
