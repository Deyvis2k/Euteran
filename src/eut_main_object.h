#ifndef EUT_MAIN_OBJECT_H
#define EUT_MAIN_OBJECT_H

#include "eut_utils.h"
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EUTERAN_TYPE_MAIN_OBJECT (euteran_main_object_get_type())

G_DECLARE_FINAL_TYPE(EuteranMainObject, euteran_main_object, EUTERAN, MAIN_OBJECT, GObject) 

EuteranMainObject *euteran_main_object_new(void);
void euteran_main_object_add_widget(EuteranMainObject *self, GtkWidget *widget, const gchar *name);
void euteran_main_object_remove_widget(EuteranMainObject *self, GtkWidget *widget);
void euteran_main_object_clear(EuteranMainObject *self);
GObject *euteran_main_object_get_widget_from_name(EuteranMainObject *self, GtkBuilder *builder, const gchar *name); 
GObject *euteran_main_object_get_widget_at(EuteranMainObject *self, WIDGETS); 
void euteran_main_object_get_widget(EuteranMainObject *self, GtkWidget *widget);
void euteran_main_object_autoinsert(EuteranMainObject *self, GtkBuilder *builder);
void euteran_main_object_clear(EuteranMainObject *self);
void euteran_main_object_set_duration(EuteranMainObject *self, double duration);
void euteran_main_object_set_timer(EuteranMainObject *self, GTimer *timer);
void euteran_main_object_set_offset_time(EuteranMainObject *self, double offset_time);
double euteran_main_object_get_duration(EuteranMainObject *self);
GTimer *euteran_main_object_get_timer(EuteranMainObject *self);
double euteran_main_object_get_offset_time(EuteranMainObject *self);
GList *euteran_main_object_get_widgets_list(EuteranMainObject *self);
gboolean euteran_main_object_is_valid(EuteranMainObject *self);
void euteran_main_object_set_optional_pointer_object(EuteranMainObject *self, void *optional_pointer_object);
void *euteran_main_object_get_optional_pointer_object(EuteranMainObject *self);
void euteran_main_object_save_data_json(EuteranMainObject *self);
void euteran_main_object_load_and_apply_data_json(EuteranMainObject *self, PlayMusicFunc play_music_func);
GCancellable *euteran_main_object_get_cancellable(EuteranMainObject *self);
void euteran_main_object_set_cancellable(EuteranMainObject *self, GCancellable *cancellable);
void euteran_main_object_reset_cancellable(EuteranMainObject *self);
GtkWidget*
euteran_main_object_get_list_box
(
    GtkSelectionModel *pages,
    guint             index
);


typedef struct{
    const gchar *music_path;
    const gchar *music_duration;
    const gchar *music_duration_raw;
} MusicDataSave;

typedef struct {
    gchar *name;
    GList *music_list;
} MusicListData;

G_END_DECLS

#endif 

