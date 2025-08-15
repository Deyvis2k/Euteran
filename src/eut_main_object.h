#pragma once

#include "eut_settings.h"
#include "eut_subwindow.h"
#include <glib-object.h>
#include <gtk/gtk.h>

enum EuteranTheme{
    EUTERAN_LIGHTMODE = 1,
    EUTERAN_DARKMODE = -1
};

typedef struct {
    GtkCssProvider *provider;
    GtkSettings *theme_settings;
    enum EuteranTheme theme;
} EuteranThemeData;
    
G_BEGIN_DECLS

#define TYPE_EUTERAN_MAIN_OBJECT (euteran_main_object_get_type())

G_DECLARE_FINAL_TYPE(EuteranMainObject, euteran_main_object, EUTERAN, MAIN_OBJECT, AdwApplicationWindow) 

EuteranMainObject*  euteran_main_object_new(void);
GtkCssProvider*     euteran_main_object_get_css_provider                  (EuteranMainObject* self);
EuteranThemeData*   euteran_main_object_get_theme_data                    (EuteranMainObject* self);
GTimer*             euteran_main_object_get_timer                         (EuteranMainObject* self);
gboolean            euteran_main_object_is_valid                          (EuteranMainObject* self);
GCancellable*       euteran_main_object_get_cancellable                   (EuteranMainObject* self);
gboolean            euteran_main_object_list_box_contains_music           (EuteranMainObject* self, const gchar* music_to_analyze);
GtkWidget*          euteran_main_object_get_index_list_box                (GtkSelectionModel* pages, guint index);
GtkSettings*        euteran_main_object_get_theme_settings                (EuteranMainObject* self);
EutSubwindow*       euteran_main_object_get_subwindow                     (EuteranMainObject* self);
GtkListBox*         euteran_main_object_get_focused_list_box              (EuteranMainObject* self);
GtkGrid*            euteran_main_object_get_music_display_content         (EuteranMainObject* self);
GtkScale*           euteran_main_object_get_progress_bar                  (EuteranMainObject* self);
GtkScale*           euteran_main_object_get_volume_slider                 (EuteranMainObject* self);
GtkButton*          euteran_main_object_get_music_button                  (EuteranMainObject* self);
GtkListBox*         euteran_main_object_get_list_box                      (EuteranMainObject* self);
GtkMenuButton*      euteran_main_object_get_menu_button                   (EuteranMainObject* self);
GtkStack*           euteran_main_object_get_stack                         (EuteranMainObject* self);
GtkStackSwitcher*   euteran_main_object_get_stack_switcher                (EuteranMainObject* self);
GtkButton*          euteran_main_object_get_switcher_add_button           (EuteranMainObject* self);
GtkButton*          euteran_main_object_get_stop_button                   (EuteranMainObject* self);
GtkScale*           euteran_main_object_get_input_slider                  (EuteranMainObject* self);
GtkButton*          euteran_main_object_get_input_button                  (EuteranMainObject* self);
GtkButton*          euteran_main_object_get_stop_recording_button         (EuteranMainObject* self);
GtkButton*          euteran_main_object_get_alter_theme_button            (EuteranMainObject* self);
GtkButton*          euteran_main_object_get_button_input_recording        (EuteranMainObject* self);
GtkButton*          euteran_main_object_get_button_input_stop_recording   (EuteranMainObject* self);
EuteranSettings*    euteran_main_object_get_settings_object               (EuteranMainObject* self);
void*               euteran_main_object_get_binary_parser                 (EuteranMainObject* self);
void*               euteran_main_object_get_optional_pointer_object       (EuteranMainObject* self);
double              euteran_main_object_get_duration                      (EuteranMainObject* self);
double              euteran_main_object_get_offset_time                   (EuteranMainObject* self);

void                euteran_main_object_set_optional_pointer_object       (EuteranMainObject* self, void* optional_pointer_object);
void                euteran_main_object_add_widget                        (EuteranMainObject* self, GtkWidget* widget, const gchar* name);
void                euteran_main_object_remove_widget                     (EuteranMainObject* self, GtkWidget* widget);
void                euteran_main_object_set_cancellable                   (EuteranMainObject* self, GCancellable* cancellable);
void                euteran_main_object_reset_cancellable                 (EuteranMainObject* self);
void                euteran_main_object_autoinsert                        (EuteranMainObject* self);
void                euteran_main_object_set_duration                      (EuteranMainObject* self, double duration);
void                euteran_main_object_set_timer                         (EuteranMainObject* self, GTimer* timer);
void                euteran_main_object_set_offset_time                   (EuteranMainObject* self, double offset_time);
void                euteran_main_object_switch_theme                      (EuteranMainObject* self, GtkWidget* button); 
G_END_DECLS
