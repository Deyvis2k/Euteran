#pragma once

#include <gtk/gtk.h>
#include "eut_main_object.h"


static guint progress_timer_id = 0;

gboolean on_drop(
    EuteranMainObject *wd,
    const GValue *value,
    double x,
    double y,
    GtkDropTarget *target
);

gboolean on_clicked_progress_bar(
    EuteranMainObject *wd,
    GtkScrollType scroll,
    gdouble value,
    GtkRange* self
);


void on_window_destroy(EuteranMainObject *wd);
gboolean on_volume_changed(EuteranMainObject *wd, GtkRange *range);
void play_audio_task(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
gboolean update_progress_bar(gpointer user_data);
void pause_audio(EuteranMainObject *wd, GtkButton *button);
void play_selected_music(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);
gboolean on_switcher_add_button(EuteranMainObject *wd, GtkButton *button);
void on_stack_switcher_right_click(EuteranMainObject *wd, gint n_press, double x, double y, GtkGestureClick *gesture);
void interrupt_audio(EuteranMainObject *wd, GtkButton *button);
gboolean on_clicked_input_slider(
    GtkRange* self,
    GtkScrollType scroll,
    gdouble value,
    gpointer user_data
);
void start_recording_input(EuteranMainObject *wd, GtkButton *button);

void 
monitor_audio_dir_linkfiles(
    const gchar *dir,
    gpointer user_data
);

void 
stop_audio_input_recording(
    EuteranMainObject *wd,
    GtkButton      *button
);
