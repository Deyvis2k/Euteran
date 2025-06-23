#include "widget_properties.h"
#include "e_commandw.h"
#include "e_logs.h"
#include "e_widgets.h"
#include "constants.h"
#include "gio/gio.h"
#include "gtk/gtk.h"
#include "utils.h"



void play_audio_task(
    GTask           *task, 
    gpointer        source_object, 
    gpointer        task_data, 
    GCancellable    *cancellable
) 
{
    AudioTaskData *data = (AudioTaskData *) g_task_get_task_data(task);
    if (!data) {
        printf("Error: no filename\n");
        return;
    }
    play_audio(data->filename, data->volume, cancellable, &paused, NULL);
}

void on_volume_changed(GtkRange *range, gpointer user_data) {
    if(!user_data) return;
    set_last_volume(gtk_range_get_value(GTK_RANGE(range)));
    ((volume_data *)user_data)->volume = get_last_volume();
}

void create_slider(GtkWidget *slider, volume_data *volume) {
    if(!GTK_IS_RANGE(slider) || !volume) {
        printf("Erro: falha ao criar slider\n");
        return;
    }
    g_signal_handlers_disconnect_by_func(slider, (gpointer)on_volume_changed, NULL);
    g_signal_connect(slider, "value-changed", G_CALLBACK(on_volume_changed), volume);
    gtk_range_set_value(GTK_RANGE(slider), get_last_volume());
}

void on_task_completed(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GTask *task = G_TASK(source_object);
    if (G_IS_TASK(task)) {
        g_object_unref(task); 
    }
    log_message("Task finalizada e recursos liberados.");
}

gboolean update_progress_bar(gpointer user_data) {
    WidgetsData *data = (WidgetsData *)user_data;
    if (!data || !GTK_IS_RANGE(data->progress_bar)) {
        progress_timer_id = 0;
        return G_SOURCE_REMOVE; 
    }


    if (data->elapsed_time >= data->music_duration) {
        gtk_range_set_value(GTK_RANGE(data->progress_bar), 1.0);
        progress_timer_id = 0;
        return G_SOURCE_REMOVE;
    }

    gdouble fraction = data->elapsed_time / data->music_duration;
    gtk_range_set_value(GTK_RANGE(data->progress_bar), fraction);

    if(paused == FALSE)
        data->elapsed_time += 0.1;
    return G_SOURCE_CONTINUE;
}

gboolean pause_audio(GtkWidget *button, gpointer user_data) {
    WidgetsData *data = (WidgetsData *)user_data;
    struct data *audio_data = g_object_get_data(G_OBJECT(button), "audio_data");
    g_mutex_lock(&paused_mutex);
    paused = !paused;
    if(audio_data && audio_data->stream){
        pw_stream_set_active(audio_data->stream, paused);
    }
    g_mutex_unlock(&paused_mutex);
    if (paused) {
        gtk_button_set_icon_name(GTK_BUTTON(button), "media-playback-pause");
    } else {
        gtk_button_set_icon_name(GTK_BUTTON(button), "media-playback-start");
    }

    return FALSE;
}

gboolean on_value_changed_music(GtkRange *range, gpointer user_data) {
    if(!user_data) return FALSE;
    WidgetsData *widgets_data = (WidgetsData *)user_data;
    g_mutex_lock(&paused_mutex);
    if (current_task && !g_cancellable_is_cancelled(current_cancellable)) {
        AudioTaskData *data = g_task_get_task_data(current_task);
        if(data){
            
        }
    }
    g_mutex_unlock(&paused_mutex);
    return FALSE;
}

void play_selected_music(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    if (row == NULL || user_data == NULL) {
        printf("Erro: falha ao executar play_selected_music\n");
        return;
    }
    GtkWidget *box_child = gtk_list_box_row_get_child(row);

    GtkWidget *label = gtk_widget_get_first_child(box_child);
    if (!GTK_IS_LABEL(label)) {
        printf("Erro: O primeiro widget do GtkBox não é um GtkLabel\n");
        return;
    }

    const char *filename = gtk_label_get_text(GTK_LABEL(label));
    if (!filename || strlen(filename) == 0) {
        printf("Erro: Nome da música inválido\n");
        return;
    }
    
    GtkWidget *music_label = gtk_widget_get_last_child(box_child);
    if (!GTK_IS_LABEL(music_label)) {
        printf("Erro: O segundo widget do GtkBox não é um GtkLabel\n");
        return;
    }

    const char *music_name = gtk_label_get_text(GTK_LABEL(music_label));
    if(!music_name || strlen(music_name) == 0) {
        printf("Erro: Nome da musica inválido\n");
        return;
    }

    WidgetsData *widgets_data = (WidgetsData *)user_data;
    if (!widgets_data) {
        printf("Erro: Falha ao acessar WidgetsData\n");
        return;
    }

    if (!GTK_IS_RANGE(widgets_data->progress_bar) || !GTK_IS_RANGE(widgets_data->volume_slider)) {
        printf("Erro: falha ao alocar WidgetsData\n");
        return;
    }

    AudioTaskData *data = g_new0(AudioTaskData, 1);
    if (!data) {
        printf("Erro: falha ao alocar AudioTaskData\n");
        return;
    }

    if (current_cancellable && !g_cancellable_is_cancelled(current_cancellable)) {
        g_cancellable_cancel(current_cancellable);
    }

    data->volume = g_new0(volume_data, 1);
    if (!data->volume) {
        printf("Erro: falha ao alocar volume_data\n");
        g_free(data);
        return;
    }

    paused = FALSE;
    gtk_button_set_icon_name(GTK_BUTTON(widgets_data->music_button), "media-playback-start");
    data->volume->volume = get_last_volume();
    snprintf(data->filename, sizeof(data->filename), "%s%s", SYM_AUDIO_DIR, filename);
    data->music_duration = string_to_double(g_object_get_data(G_OBJECT(row), "music_duration"));
    create_slider(GTK_WIDGET(widgets_data->volume_slider), data->volume);
    gtk_range_set_value(GTK_RANGE(widgets_data->progress_bar), 0.0);
    widgets_data->music_duration = data->music_duration;
    widgets_data->elapsed_time = 0.0;

    if (progress_timer_id != 0) {
        if (g_source_remove(progress_timer_id) == 0) {
            g_print("Timer removido\n");
        }
        progress_timer_id = 0;
    }

    GTask *old_task = NULL;
    GCancellable *old_cancellable = NULL;

    static GMutex mutex;
    g_mutex_lock(&mutex);

    if (current_cancellable && !g_cancellable_is_cancelled(current_cancellable)) {
        printf(RED_COLOR "Cancelando áudio atual...\n" RESET_COLOR);
        g_cancellable_cancel(current_cancellable);
        old_cancellable = current_cancellable;
        current_cancellable = NULL;
    }

    if (current_task) {
        old_task = current_task;
        current_task = NULL;
    }

    current_cancellable = g_cancellable_new();
    current_task = g_task_new(NULL, current_cancellable, on_task_completed, NULL);
    g_task_set_task_data(current_task, data, g_free);
    g_task_run_in_thread(current_task, play_audio_task);
    g_task_set_check_cancellable(current_task, TRUE);
    progress_timer_id = g_timeout_add(100, update_progress_bar, widgets_data);
    g_mutex_unlock(&mutex);

    if (old_cancellable) g_object_unref(old_cancellable);
    if (old_task) g_object_unref(old_task);
}

gboolean on_drop(
    GtkDropTarget *target,
    const GValue *value,
    double x,
    double y,
    gpointer user_data
) {
    GdkFileList *file_list = g_value_get_boxed (value);
    GSList *list = gdk_file_list_get_files (file_list);
    for (GSList *l = list; l != NULL; l = l->next)
    {
        GFile* file = l->data;
        const gchar *path = g_file_get_path (file);
        add_music_to_list(
            user_data, 
            path, 
            play_selected_music);
    }
    return TRUE;
}



void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    static GMutex mutex;
    g_mutex_lock(&mutex);
    if (progress_timer_id != 0) {
        if(g_source_remove(progress_timer_id) == 0){
            g_print("Timer removido\n");
        }
        progress_timer_id = 0;
    }
    g_mutex_unlock(&mutex);
    if (user_data) {
        g_free(user_data);
    }

    save_current_settings(get_last_volume());
}

gboolean on_clicked_progess_bar(
    GtkRange*       self,
    GtkScrollType*  scroll,
    gdouble         value,
    gpointer        user_data
){
    return TRUE;
}



static void
on_audio_link_changed (
    GFileMonitor       *monitor,
    GFile              *file,
    GFile              *other_file,
    GFileMonitorEvent  event_type,
    gpointer           user_data
)
{
    WidgetsData *wd = (WidgetsData *)user_data;
    if (!(event_type == G_FILE_MONITOR_EVENT_DELETED ||
          event_type == G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED))
        return;

    GtkListBox *list_box = GTK_LIST_BOX (wd->list_box);
    if (!list_box) return;

    g_autofree gchar *changed_name = g_file_get_basename (file);

    for (GtkWidget *row = gtk_widget_get_first_child (GTK_WIDGET(list_box));
         row != NULL;
         row = gtk_widget_get_next_sibling (row))
    {
        GtkWidget *row_box = gtk_widget_get_first_child (row);
        if (!GTK_IS_BOX (row_box))
            continue;
        GtkWidget *label = gtk_widget_get_first_child (row_box);
        if (!GTK_IS_LABEL (label))
            continue;
        const gchar *label_text = gtk_label_get_text (GTK_LABEL(label));
        g_debug ("comparing %s with %s", label_text, changed_name);
        if (g_strcmp0 (label_text, changed_name) == 0) {
            g_message ("removing %s", changed_name);
            gtk_list_box_remove (list_box, row);
            break;
        }
    }
}

void monitor_audio_dir_linkfiles(
    const gchar     *audio_link_dir,
    gpointer        user_data
)
{
    GFile *audio_link = g_file_new_for_path(audio_link_dir);
    GError *error = NULL;
    GFileMonitor *monitor = g_file_monitor_directory(
        audio_link,
        G_FILE_MONITOR_NONE,
        NULL,
        &error
    );
    
    if(!monitor){
        log_error("Error creating monitor: %s", error->message);
        g_error_free(error);
        g_object_unref(audio_link);
        return;
    }

    g_signal_connect(
        monitor,
        "changed",
        G_CALLBACK(on_audio_link_changed),
        user_data        
    );
    g_object_unref(audio_link);
}

static void
on_audio_dir_changed (
    GFileMonitor       *monitor,
    GFile              *file,
    GFile              *other_file,
    GFileMonitorEvent  event_type,
    gpointer           user_data
)
{
    if (!(event_type == G_FILE_MONITOR_EVENT_DELETED ||
          event_type == G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED))
            return;
    
    WidgetsData *wd = (WidgetsData *)user_data;

    if(!wd) return;

    GtkListBox *list_box = GTK_LIST_BOX (wd->list_box);
    if (!list_box) return;

    gtk_list_box_remove_all(list_box);
    
    
    log_command(
        "Looks like audio dir changed, creating new audio dir: %s",
        SYM_AUDIO_DIR
    );

    run_subprocess_async(
        g_strdup_printf(
            "mkdir -p %s",
            SYM_AUDIO_DIR
        ),
        NULL,
        NULL
    );
    const gchar *path = g_object_get_data(G_OBJECT(monitor), "path_dir");
    if(!path) {
        log_error("Path not found at on_audio_dir_changed");
    } else{
        monitor_audio_dir(path, user_data);
    }
    g_signal_handlers_disconnect_by_func(
        monitor,
        G_CALLBACK(on_audio_dir_changed),
        user_data
    );
    g_object_unref(monitor);
}

void monitor_audio_dir(
    const gchar     *audio_dir,
    gpointer        user_data
)
{
    GFile *audio_dir_file = g_file_new_for_path(audio_dir);
    GError *error = NULL;
    GFileMonitor *monitor = g_file_monitor_directory(
        audio_dir_file,
        G_FILE_MONITOR_NONE,
        NULL,
        &error
    );
    
    if(!monitor){
        log_error("Error creating monitor: %s", error->message);
        g_error_free(error);
        g_object_unref(audio_dir_file);
        return;
    }

    g_signal_connect(
        monitor,
        "changed",
        G_CALLBACK(on_audio_dir_changed),
        user_data        
    );
    g_object_set_data(
        G_OBJECT(monitor),
        "path_dir",
        (gpointer)audio_dir
    );
    g_object_unref(audio_dir_file);
}
