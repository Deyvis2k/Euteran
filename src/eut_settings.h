#ifndef EUT_SETTINGS_H
#define EUT_SETTINGS_H 

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EUTERAN_TYPE_SETTINGS (euteran_settings_get_type())

G_DECLARE_FINAL_TYPE(EuteranSettings, euteran_settings, EUTERAN, SETTINGS, GObject)


EuteranSettings *euteran_settings_new(void);
void euteran_settings_save(EuteranSettings *self, GtkWindow *window);
int euteran_settings_get_window_width(EuteranSettings *self);
int euteran_settings_get_window_height(EuteranSettings *self);
float euteran_settings_get_last_volume(EuteranSettings *self);
void euteran_settings_set_last_volume(EuteranSettings *self, float volume);
EuteranSettings *euteran_settings_get(void);

G_END_DECLS

#endif
