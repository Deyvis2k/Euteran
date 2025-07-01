#pragma once

#include <gtk/gtk.h>

static GTask *current_task = NULL;
static GCancellable *current_cancellable = NULL;
static guint progress_timer_id = 0;

gboolean on_drop(
    GtkDropTarget *target,
    const GValue *value,
    double x,
    double y,
    gpointer user_data
);

gboolean on_clicked_progress_bar(
    GtkRange* self,
    GtkScrollType scroll,
    gdouble value,
    gpointer user_data
);


void on_window_destroy(GtkWidget *widget, gpointer user_data);
gboolean on_volume_changed(GtkRange *range, gpointer user_data);
void play_audio_task(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
gboolean update_progress_bar(gpointer user_data);
gboolean pause_audio(GtkWidget *button, gpointer user_data);
void play_selected_music(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);



void 
monitor_audio_dir_linkfiles(
    const gchar *dir,
    gpointer user_data
);
