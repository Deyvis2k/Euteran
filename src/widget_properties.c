#include "widget_properties.h"
#include "audio.h"
#include "e_commandw.h"
#include "e_logs.h"
#include "e_widgets.h"
#include "constants.h"
#include "euteran_main_object.h"
#include "gio/gio.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "utils.h"
#include "euteran_settings.h"

typedef struct{
    gboolean *delay_protect;
    float timer_to_consume;
} DelayData;

void play_audio_task(
    GTask          *task,
    gpointer       source_object,
    gpointer       task_data,
    GCancellable  *cancellable
) 
{
    EuteranMainAudio *data = (EuteranMainAudio *) g_task_get_task_data(task);
    if (!data) {
        printf("Error: no filename\n");
        return;
    }
    if(!cancellable){
        printf("Error: no cancellable\n");
        return;
    }
    play_audio(data);
}

gboolean on_volume_changed(GtkRange *range, gpointer user_data) {
    EuteranSettings *current_settings_singleton = euteran_settings_get();
    euteran_settings_set_last_volume(current_settings_singleton, gtk_range_get_value(range));
    return FALSE;
}

void on_task_completed(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GTask *task = G_TASK(res);
    if (task != NULL && G_IS_TASK(task)) {
        EuteranMainAudio *audio_data = g_task_get_task_data(task);
        if(audio_data){
            log_info("Começando limpeza de recursos.");
            cleanup_process(audio_data);
        }
        g_object_unref(task); 
        task = NULL;
    }
    log_message("Task finalizada e recursos liberados.");
}

gboolean pause_audio(GtkWidget *button, gpointer user_data) {
    EuteranMainObject *data = (EuteranMainObject *)user_data;
    GtkWidget *window = GTK_WIDGET(euteran_main_object_get_widget_at(data, WINDOW_PARENT));

    EuteranMainAudio *audio_data = g_task_get_task_data(current_task);
    g_return_val_if_fail(audio_data != NULL, FALSE);

    g_mutex_lock(&audio_data->paused_mutex);
    audio_data->paused = !audio_data->paused;
    g_mutex_unlock(&audio_data->paused_mutex);
    if (audio_data->paused) {
        g_timer_stop(euteran_main_object_get_timer(data));
        gtk_button_set_icon_name(GTK_BUTTON(button), "media-playback-pause-symbolic");
    } else {
        g_timer_continue(euteran_main_object_get_timer(data));
        gtk_button_set_icon_name(GTK_BUTTON(button), "media-playback-start-symbolic");
    }

    return FALSE;
}

gboolean interrupt_audio(GtkWidget *button, gpointer user_data) {
    EuteranMainObject *data = (EuteranMainObject *)user_data;
    GtkWidget *stop_button = GTK_WIDGET(euteran_main_object_get_widget_at(data, STOP_BUTTON));
    if (!GTK_IS_BUTTON(stop_button)) return FALSE;

    EuteranMainAudio *audio_data = g_task_get_task_data(current_task);
    g_return_val_if_fail(audio_data != NULL, FALSE);

    g_mutex_lock(&audio_data->paused_mutex);
    audio_data->loop_stopped = TRUE;
    g_mutex_unlock(&audio_data->paused_mutex);


    GtkWidget *progress_bar = GTK_WIDGET(euteran_main_object_get_widget_at(data, PROGRESS_BAR));
    if (GTK_IS_RANGE(progress_bar)) {
        g_timer_stop(euteran_main_object_get_timer(data));
        gtk_range_set_value(GTK_RANGE(progress_bar), 0);
    }

    return FALSE;
}

gboolean update_progress_bar(gpointer user_data) {
    EuteranMainObject *wd = (EuteranMainObject *)user_data;
    g_return_val_if_fail(
        euteran_main_object_is_valid(wd),
        G_SOURCE_REMOVE
    );

    GtkWidget *progress_bar = GTK_WIDGET(euteran_main_object_get_widget_at(wd, PROGRESS_BAR));
    if (!GTK_IS_RANGE(progress_bar)) return G_SOURCE_REMOVE;
    
    GtkWidget *window = GTK_WIDGET(euteran_main_object_get_widget_at(wd, WINDOW_PARENT));
    if (!GTK_IS_WINDOW(window)) return G_SOURCE_REMOVE;

    EuteranMainAudio *audio_data = NULL;
    if(current_task && G_IS_TASK(current_task)) 
        audio_data = g_task_get_task_data(current_task);
    else{
        progress_timer_id = 0;
        return G_SOURCE_REMOVE;
    }
        

    if (!audio_data || !audio_data->stream || audio_data->loop_stopped) {
        progress_timer_id = 0;
        return G_SOURCE_REMOVE;
    }

    if (!audio_data->task_data) {
        progress_timer_id = 0;
        return G_SOURCE_REMOVE;
    }

    if (
        (audio_data->task_data->audio_type == MP3 && !audio_data->mpg) ||
        (audio_data->task_data->audio_type == OGG && !audio_data->vorbis)
    ) {
        progress_timer_id = 0;
        return G_SOURCE_REMOVE;
    }
    GTimer *timer = euteran_main_object_get_timer(wd);
    gdouble elapsed = g_timer_elapsed(timer, NULL) + euteran_main_object_get_offset_time(wd);

    if (elapsed >= euteran_main_object_get_duration(wd)) {
        gtk_range_set_value(GTK_RANGE(progress_bar), 1.0);
        progress_timer_id = 0;
        return G_SOURCE_REMOVE;
    }

    gdouble fraction = elapsed / euteran_main_object_get_duration(wd);
    gtk_range_set_value(GTK_RANGE(progress_bar), fraction);

    return G_SOURCE_CONTINUE;
}

void 
play_selected_music(
    GtkListBox    *box, 
    GtkListBoxRow *row, 
    gpointer      user_data
)
{
    g_return_if_fail(row != NULL && user_data != NULL);

    GtkWidget *box_child = gtk_list_box_row_get_child(row);
    GtkWidget *label = gtk_widget_get_first_child(box_child);
    GtkWidget *music_label = gtk_widget_get_last_child(box_child);
    g_return_if_fail(GTK_IS_LABEL(label) && GTK_IS_LABEL(music_label));

    const char *filename = gtk_label_get_text(GTK_LABEL(label));
    const char *music_duration_str = gtk_label_get_text(GTK_LABEL(music_label));
    gchar *full_path = g_strconcat(SYM_AUDIO_DIR, filename, NULL);
    //see if filename exists in SYM_AUDIO_DIR
    g_return_if_fail(filename && *filename && music_duration_str && *music_duration_str && g_file_test(full_path, G_FILE_TEST_EXISTS));

    g_free(full_path);

    EuteranMainObject *wd = (EuteranMainObject *)user_data;
    GtkWidget *progress_bar = GTK_WIDGET(euteran_main_object_get_widget_at(wd, PROGRESS_BAR));
    GtkWidget *volume_slider = GTK_WIDGET(euteran_main_object_get_widget_at(wd, VOLUME_SLIDER));
    g_return_if_fail(GTK_IS_RANGE(progress_bar) && GTK_IS_RANGE(volume_slider));

    if (current_cancellable) {
        g_cancellable_cancel(current_cancellable);
        g_clear_object(&current_cancellable);
    }

    if (progress_timer_id != 0) {
        if (!g_source_remove(progress_timer_id)) {
            log_error("Erro ao remover progress_timer_id");
        }
        progress_timer_id = 0;
    }

    EuteranAudioTaskData *actual_data = g_new0(EuteranAudioTaskData, 1);
    g_return_if_fail(actual_data != NULL);
    

    if(g_str_has_suffix(filename, ".mp3")){
        actual_data->audio_type = MP3;
    } else if(g_str_has_suffix(filename, ".ogg")){
        actual_data->audio_type = OGG;
    } else if(g_str_has_suffix(filename, ".wav")){
        actual_data->audio_type = WAV;
    }
    else{
        log_error("Audio type not found\n");
        return;
    }

    gtk_button_set_icon_name(GTK_BUTTON(euteran_main_object_get_widget_at(wd, MUSIC_BUTTON)), "media-playback-start");

    EuteranSettings *current_settings_singleton = euteran_settings_get();
    euteran_settings_set_last_volume(current_settings_singleton, (float)gtk_range_get_value(GTK_RANGE(volume_slider)));

    snprintf(actual_data->filename, sizeof(actual_data->filename), "%s%s", SYM_AUDIO_DIR, filename);
    actual_data->music_duration = string_to_double(g_object_get_data(G_OBJECT(row), "music_duration"));

    euteran_main_object_set_duration(wd, actual_data->music_duration);
    euteran_main_object_set_offset_time(wd, 0.0);
    euteran_main_object_set_timer(wd, g_timer_new());

    gtk_range_set_value(GTK_RANGE(progress_bar), 0.0);

    EuteranMainAudio *audio_data_main = g_new0(EuteranMainAudio, 1);
    audio_data_main->task_data = actual_data;

    current_cancellable = g_cancellable_new();
    audio_data_main->cancellable = current_cancellable;



    if(!audio_data_main              ||
       !audio_data_main->cancellable ||
       !audio_data_main->task_data
    ){
        log_error("Audio data at on_clicked_progress_bar is null at the moment\n");
        return;
    }

    if(current_task && G_IS_TASK(current_task)){
        g_object_unref(current_task); 
        current_task = NULL;
    }

    

    current_task = g_task_new(NULL, current_cancellable, on_task_completed, NULL);
    g_object_ref(current_task);
    g_task_set_task_data(current_task, audio_data_main, NULL);
    g_task_set_check_cancellable(current_task, TRUE);
    g_task_run_in_thread(current_task, play_audio_task);

    progress_timer_id = g_timeout_add(100, update_progress_bar, wd);
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
        EuteranMainObject *widgets_data = (EuteranMainObject *)user_data;

        if(current_task){
            g_cancellable_cancel(current_cancellable);
            g_clear_object(&current_cancellable);
        }
        EuteranSettings *current_settings_singleton = euteran_settings_get();
        euteran_settings_save(current_settings_singleton, GTK_WINDOW(euteran_main_object_get_widget_at(widgets_data, WINDOW_PARENT)));
        euteran_main_object_save_data_json(widgets_data);
        euteran_main_object_clear(widgets_data);   
    }
}

static gboolean 
on_delay_timer_clicked(
    gpointer user_data
)
{
    DelayData *data = (DelayData *)user_data;

    if (data->timer_to_consume <= 2.0f) {
        data->timer_to_consume += 0.1f;
        return G_SOURCE_CONTINUE;
    }

    *(data->delay_protect) = FALSE;
    g_free(data);
    return G_SOURCE_REMOVE;
}

gboolean on_clicked_progress_bar(
    GtkRange*       self,
    GtkScrollType   scroll, 
    gdouble         value,
    gpointer        user_data
){
    if(scroll != 1){
        return TRUE;
    }

    EuteranMainObject *wd = (EuteranMainObject *)user_data;
    if(!wd){
        log_error("WD does not exist");
        return TRUE;
    }
    EuteranMainAudio *audio_data = NULL;
    if(current_task != NULL && G_IS_TASK(current_task)){
        audio_data = g_task_get_task_data(current_task);
    }else{
        log_error("Current task does not exist");
        return TRUE;
    }
        

    if(!audio_data || !audio_data->stream){
        log_error("Audio data or stream does not exist");
        return TRUE;
    } 

    if(!audio_data->task_data){
        log_error("task data does not exist");
        return TRUE;
    }

    g_return_val_if_fail(
        (audio_data->task_data->audio_type == OGG && audio_data->vorbis != NULL) ||
        (audio_data->task_data->audio_type == MP3 && audio_data->mpg != NULL) ||
        (audio_data->task_data->audio_type == WAV && audio_data->wav != NULL),
        TRUE
    );
    
    //lembrete -> isso aqui foi feito para proteger 
    // o progress bar de ser movido rapidamente, causando problemas 
    // de audio (principalmente com ogg), vou ver de consertar depois
    static guint delay_timer_clicked_id = 0;
    static gboolean delay_protect = FALSE;
    if (delay_protect) {
        return TRUE;
    }
    delay_protect = TRUE;

    DelayData *data = g_new0(DelayData, 1);
    data->delay_protect = &delay_protect;
    data->timer_to_consume = 0.0f;

    delay_timer_clicked_id = g_timeout_add(35, on_delay_timer_clicked, (gpointer)data);

    audio_data->task_data->frameoff = value * audio_data->task_data->music_duration * audio_data->rate;

    
    g_mutex_lock(&audio_data->paused_mutex);
    audio_data->paused = TRUE;

    GtkRange *progress_bar = GTK_RANGE(euteran_main_object_get_widget_at(wd, PROGRESS_BAR));
    

    if(audio_data->task_data->audio_type == OGG){
        gdouble new_time = value * audio_data->task_data->music_duration;
        euteran_main_object_set_timer(wd, g_timer_new());
        g_timer_start(euteran_main_object_get_timer(wd));
        euteran_main_object_set_offset_time(wd, new_time);
        gtk_range_set_value(progress_bar, value);
        search_on_audio(audio_data);
    }

    if(audio_data->task_data->audio_type == MP3){
        gdouble new_time = value * audio_data->task_data->music_duration;
        euteran_main_object_set_timer(wd, g_timer_new());
        g_timer_start(euteran_main_object_get_timer(wd));
        euteran_main_object_set_offset_time(wd, new_time);
        gtk_range_set_value(progress_bar, value);
        mpg123_feed(audio_data->mpg, NULL, 0);
        search_on_audio(audio_data);
    } 

    if (audio_data->task_data->audio_type == WAV) {
        gdouble new_time = value * audio_data->task_data->music_duration;
        euteran_main_object_set_timer(wd, g_timer_new());
        g_timer_start(euteran_main_object_get_timer(wd));
        euteran_main_object_set_offset_time(wd, new_time);
        gtk_range_set_value(progress_bar, value);
        search_on_audio(audio_data);
    }

    audio_data->paused = FALSE;
    g_mutex_unlock(&audio_data->paused_mutex);

    gtk_button_set_icon_name(GTK_BUTTON(euteran_main_object_get_widget_at(wd, MUSIC_BUTTON)), "media-playback-start");

    return FALSE;
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
    EuteranMainObject *wd = (EuteranMainObject *)user_data;
    if (!(event_type == G_FILE_MONITOR_EVENT_DELETED ||
          event_type == G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED))
        return;


    GtkListBox *list_box = GTK_LIST_BOX(euteran_main_object_get_widget_at(wd, LIST_BOX));
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
    if (!(event_type == G_FILE_MONITOR_EVENT_DELETED))
        return;

    const gchar *path = g_object_get_data(G_OBJECT(monitor), "path_dir");
    if (!path) {
        log_error("Path not found at on_audio_dir_changed");
        return;
    }

    gchar *deleted_path = g_file_get_path(file);
    if (!deleted_path) return;

    gboolean is_dir_deleted = g_strcmp0(deleted_path, path) == 0;
    g_free(deleted_path);

    if (!is_dir_deleted) {
        return;
    }

    EuteranMainObject *wd = (EuteranMainObject *)user_data;
    if (!wd) return;

    GtkListBox *list_box = GTK_LIST_BOX(euteran_main_object_get_widget_at(wd, LIST_BOX));
    if (!list_box) return;

    gtk_list_box_remove_all(list_box);

    log_command("Looks like audio dir changed, creating new audio dir: %s", SYM_AUDIO_DIR);

    g_mkdir_with_parents(SYM_AUDIO_DIR, 0777);


    g_signal_handlers_disconnect_by_func(
        monitor,
        G_CALLBACK(on_audio_dir_changed),
        user_data
    );

    monitor_audio_dir(SYM_AUDIO_DIR, user_data);

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


gboolean
on_switcher_add_button(
    GtkWidget       *button,
    gpointer        user_data
)
{
    EuteranMainObject *wd = (EuteranMainObject *)user_data;
    if (!wd || !EUTERAN_IS_MAIN_OBJECT(wd)) {
        return FALSE;
    }


    GtkWidget *stack = (GtkWidget *)euteran_main_object_get_widget_at(wd, STACK);


    if(!stack || !GTK_IS_STACK(stack)){
        return FALSE;
    }

    GtkSelectionModel *pages = gtk_stack_get_pages(GTK_STACK(stack));

    if(!pages || !GTK_IS_SELECTION_MODEL(pages)){
        return FALSE;
    }

    guint n_items = g_list_model_get_n_items(G_LIST_MODEL(pages));
    GtkStackPage *page_ = g_list_model_get_item(G_LIST_MODEL(pages), n_items - 1);
    if (!page_ || !GTK_IS_STACK_PAGE(page_)) {
        return FALSE;
    }

    g_object_unref(page_); 


    const gchar *last_page_title = gtk_stack_page_get_title(GTK_STACK_PAGE(page_));

    if(!last_page_title || !GTK_IS_STACK_PAGE(page_)) {
        return FALSE;
    }

    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled_window),
        GTK_POLICY_NEVER,
        GTK_POLICY_AUTOMATIC
    );
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(scrolled_window),
        297
    );
    
    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_BROWSE);
    gtk_widget_add_css_class(list_box, "list_box");

    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scrolled_window),
        list_box
    );



    GtkStackPage *page = gtk_stack_add_child(
        GTK_STACK(stack),
        scrolled_window
    );
    gint last_page_index = atoi(last_page_title);
    char *page_name = g_strdup_printf("%d",  last_page_index + 1);
    gtk_stack_page_set_title(page, page_name);
    g_free(page_name);

    return TRUE;

}


void
on_stack_switcher_right_click(
    GtkGestureClick *gesture, 
    gint            n_press,
    double          x, 
    double          y, 
    gpointer        user_data
)
{
    if(gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture)) == GDK_BUTTON_SECONDARY){
        EuteranMainObject *wd = (EuteranMainObject *)user_data;
        if (!wd || !EUTERAN_IS_MAIN_OBJECT(wd)) {
            log_error("WD does not exist");
            return;
        }
        GtkWidget *switcher = gtk_event_controller_get_widget(
            GTK_EVENT_CONTROLLER((gesture))
        );
        GtkWidget *pressed_button = gtk_widget_pick(
            switcher,
            x,
            y,
            GTK_PICK_DEFAULT
        );
        
        if (pressed_button == NULL) return;

        GtkSelectionModel *pages = gtk_stack_get_pages(
            GTK_STACK(euteran_main_object_get_widget_at(wd, STACK))
        );

        GtkWidget *label_to_remove = NULL;

        if(GTK_IS_LABEL(pressed_button)){
            label_to_remove = pressed_button;
        } else if(GTK_IS_TOGGLE_BUTTON(pressed_button)){
            label_to_remove = gtk_widget_get_first_child(pressed_button);
        }

        const gchar *page_name = gtk_label_get_text(GTK_LABEL(label_to_remove));
        GtkWidget *stack_page = NULL;
        guint n_pages = g_list_model_get_n_items(G_LIST_MODEL(pages));
        guint index_to_remove = 0;

        for (guint i = 0; i < n_pages; i++) {
            const gchar *page_title = gtk_stack_page_get_title(
                GTK_STACK_PAGE(g_list_model_get_item(G_LIST_MODEL(pages), i))
            );
            if(g_strcmp0(page_name, page_title) == 0){
                stack_page = gtk_stack_page_get_child(
                    GTK_STACK_PAGE(g_list_model_get_item(G_LIST_MODEL(pages), i))
                );
                index_to_remove = i;
                break;
            }
        }
        log_warning("Removing page %s", page_name);
        if(stack_page){
            GtkWidget *list_box_to_clean = euteran_main_object_get_list_box(
                pages,
                index_to_remove);

            if(list_box_to_clean){
                GtkWidget *list_box_child = gtk_widget_get_first_child(list_box_to_clean);
                while(list_box_child){
                    gtk_widget_unparent(list_box_child);
                    list_box_child = gtk_widget_get_first_child(list_box_to_clean);
                }

                gtk_widget_queue_draw(list_box_to_clean);
            }

            gtk_stack_remove(
                GTK_STACK(euteran_main_object_get_widget_at(wd, STACK)),
                stack_page
            );
        }

        
    }
}
