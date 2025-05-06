#include <bits/sockaddr.h>
#include <mpg123.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtkshortcut.h"
#include "pipewire/stream.h"
#include "src/audio.h"
#include "src/e_widgets.h"
#include "src/ewindows.h"
#include "src/utils.h"
#include "src/constants.h"
#include "src/widgets_devices.h"

typedef struct {
    char filename[512];
    volume_data *volume;
    double music_duration;
} AudioTaskData;

static GTask *current_task = NULL;
static GCancellable *current_cancellable = NULL;
static guint progress_timer_id = 0;

static void play_audio_task(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    AudioTaskData *data = (AudioTaskData *) g_task_get_task_data(task);
    if (!data) {
        printf("Error: no filename\n");
        return;
    }
    play_audio(data->filename, data->volume, cancellable, &paused, NULL);
}

static void on_volume_changed(GtkRange *range, gpointer user_data) {
    if(!user_data) return;
    last_volume = gtk_range_get_value(GTK_RANGE(range));
    ((volume_data *)user_data)->volume = last_volume;
}

void create_slider(GtkWidget *slider, volume_data *volume) {
    if(!GTK_IS_RANGE(slider) || !volume) {
        printf("Erro: falha ao criar slider\n");
        return;
    }
    g_signal_handlers_disconnect_by_func(slider, (gpointer)on_volume_changed, NULL);
    g_signal_connect(slider, "value-changed", G_CALLBACK(on_volume_changed), volume);
    gtk_range_set_value(GTK_RANGE(slider), last_volume);
}

static void on_task_completed(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GTask *task = G_TASK(source_object);
    if (G_IS_TASK(task)) {
        g_object_unref(task); 
    }

    printf(RED_COLOR "[LOG] Task finalizada e recursos liberados.\n" RESET_COLOR);
}

static void animate_progress_bar(GtkWidget *widget, gpointer user_data) {
    if(user_data == NULL) return;
    double fraction = *(double *)user_data;

    gtk_range_set_value(GTK_RANGE(widget), fraction);
}

static gboolean update_progress_bar(gpointer user_data) {
    WidgetsData *data = (WidgetsData *)user_data;
    if (!data || !GTK_IS_RANGE(data->progress_bar)) {
        return G_SOURCE_REMOVE; 
    }

    if (data->elapsed_time >= data->music_duration) {
        gtk_range_set_value(GTK_RANGE(data->progress_bar), 1.0);
        return G_SOURCE_REMOVE;
    }

    gdouble fraction = data->elapsed_time / data->music_duration;
    gtk_range_set_value(GTK_RANGE(data->progress_bar), fraction);

    if(paused == FALSE)
        data->elapsed_time += 0.1;
    return G_SOURCE_CONTINUE;
}

static gboolean pause_audio(GtkWidget *button, gpointer user_data) {
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

static gboolean on_value_changed_music(GtkRange *range, gpointer user_data) {
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
    if (!GTK_IS_GRID(box_child)) {
        printf("Erro: O filho da row não é um GtkGrid\n");
        return;
    }

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
    
    GtkWidget *music_label = gtk_widget_get_next_sibling(label);
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
    data->volume->volume = last_volume;
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

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
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
}

gboolean is_session_a_wm(const char* session_name, const char* actual_session_name) {
    return strcmp(session_name, actual_session_name) == 0;   
} 


void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gchar *title;
    #ifdef __linux__ 
        #include <stdlib.h>
        const char* sessions_names[] = {"i3", "hyprland","sway", NULL};
        const char *session_name = getenv("DESKTOP_SESSION");
        for (int i = 0; sessions_names[i]; i++) {
            if(is_session_a_wm(sessions_names[i], session_name)) {
                title = "Background";
                break;
            }
        }
        if (title == NULL) {
            printf("Session name: %s\n", session_name);
            title = "Soundpad";
        }
    #else 
        title = "Soundpad";
    #endif
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), 650, 100);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_widget_set_hexpand(GTK_WIDGET(window), FALSE);
    gtk_widget_add_css_class(GTK_WIDGET(window), "main_window_class");

    printf("window pointer in activate %p\n", window);
    
    GFile *css_file = get_file_from_path();

    GtkWidget *main_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(main_grid), 5);
    

    WidgetsData *widgets_data = g_new0(WidgetsData, 1);
    if (!widgets_data) {
        printf("Erro: falha ao alocar WidgetsData\n");
        return;
    }

    GtkWidget* music_display_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_size_request(music_display_content, 565, 10);
    gtk_widget_set_halign(music_display_content, GTK_ALIGN_CENTER);
    GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
    gtk_widget_set_size_request(slider, 250, 4);
    gtk_widget_add_css_class(music_display_content, "music_display_content_class");
    gtk_box_append(GTK_BOX(music_display_content), slider);
    GtkWidget *progress_bar = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
    gtk_widget_add_css_class(GTK_WIDGET(progress_bar), "progress_bar_class");
    gtk_widget_set_size_request(progress_bar, 250, 4);
    gtk_box_append(GTK_BOX(music_display_content), progress_bar);
    GtkWidget *music_button = gtk_button_new_from_icon_name("media-playback-start");
    gtk_widget_add_css_class(GTK_WIDGET(music_button), "music_button_class");
    gtk_widget_set_size_request(music_button, 10, 5);
    gtk_box_append(GTK_BOX(music_display_content), music_button);
    gtk_widget_set_cursor_from_name(music_button, "hand2");

    widgets_data->music_display_content = music_display_content;
    widgets_data->volume_slider = slider;
    widgets_data->progress_bar = progress_bar;
    widgets_data->music_button = music_button;
    widgets_data->window_parent = window;

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_file(provider, css_file);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *music_holder_grid = gtk_grid_new();
    gtk_widget_set_halign(music_holder_grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(music_holder_grid, GTK_ALIGN_END);
    gtk_window_set_child(GTK_WINDOW(window), main_grid);

    GtkWidget *menu_button = gtk_menu_button_new();
    gtk_widget_set_valign(menu_button, GTK_ALIGN_START);
    gtk_widget_set_halign(menu_button, GTK_ALIGN_START);
    gtk_widget_set_size_request(menu_button, 50, 10);
    gtk_menu_button_set_label(GTK_MENU_BUTTON(menu_button), "Options");
    gtk_widget_add_css_class(menu_button, "menu_button");

    GtkWidget *popover = gtk_popover_new();
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), popover);
    gtk_widget_set_hexpand(popover, FALSE);
    gtk_widget_set_vexpand(popover, FALSE);
    gtk_popover_set_default_widget(GTK_POPOVER(popover), menu_button);
    gtk_widget_add_css_class(popover, "popover");
    
    GtkWidget *popover_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(popover_box, GTK_ALIGN_CENTER);
    gtk_popover_set_child(GTK_POPOVER(popover), popover_box);
    gtk_widget_set_hexpand(popover_box, FALSE);
    gtk_widget_set_vexpand(popover_box, FALSE);
    gtk_widget_add_css_class(popover_box, "popover_box");

    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_BROWSE);
    gtk_grid_attach(GTK_GRID(music_holder_grid), list_box, 0, 1, 1, 1);

    widgets_data->list_box = list_box;

    gtk_widget_add_css_class(widgets_data->list_box, "list_box");

    gtk_widget_add_css_class(slider, "slider_main");
    gtk_grid_attach(GTK_GRID(main_grid), menu_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), music_display_content, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), music_holder_grid, 0, 2, 1, 1);

    const char* popover_names[] = {"Select Folder", "Bindings", "Devices", NULL};
    gboolean popover_visible = FALSE;
    size_t length = sizeof(popover_names) / sizeof(popover_names[0]);
    for(int i = 0; i < length; i++){
        if(!popover_names[i]) break;
        GtkWidget *button = gtk_button_new_with_label(popover_names[i]);
        gtk_widget_set_hexpand(button, FALSE);
        gtk_widget_set_size_request(button, 70, 20);

        if(strcmp(popover_names[i], "Select Folder") == 0){
            g_signal_connect(button, "clicked", G_CALLBACK(select_folder), window);
            gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
        }

        if(strcmp(popover_names[i], "Devices") == 0){
            g_signal_connect(button, "clicked", G_CALLBACK(construct_widget), window);
            gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
        }
       
        gtk_widget_add_css_class(button, "popover_button");
        gtk_box_append(GTK_BOX(popover_box), button);
    }

    g_object_set_data(G_OBJECT(window), "play_selected_music", (gpointer)play_selected_music);
    g_object_set_data(G_OBJECT(window), "widgets_data", widgets_data);
    g_object_set_data(G_OBJECT(window), "grid_data", music_holder_grid);


    // tags 
    GtkWidget *tag_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 250);
    gtk_widget_set_size_request(tag_box, 650, 10);
    gtk_widget_set_hexpand(tag_box, FALSE);
    gtk_widget_set_vexpand(tag_box, FALSE);
    GtkWidget *tag_label_name = gtk_label_new("Music Name");
    GtkWidget *tag_label_duration = gtk_label_new("Duration");
    gtk_widget_set_margin_start(tag_label_name, 240);
    gtk_widget_set_halign(tag_label_name, GTK_ALIGN_END);
    gtk_widget_set_halign(tag_label_duration, GTK_ALIGN_START);
    gtk_widget_add_css_class(tag_box, "tag_box");
    gtk_widget_add_css_class(tag_label_name, "tag_label_name");
    gtk_widget_add_css_class(tag_label_duration, "tag_label_duration");
    gtk_grid_attach(GTK_GRID(music_holder_grid), tag_box, 0, 0, 2, 1);
    gtk_box_append(GTK_BOX(tag_box), tag_label_name);
    gtk_box_append(GTK_BOX(tag_box), tag_label_duration);
    gtk_widget_add_css_class(music_holder_grid, "music_holder_grid");


        
    if(g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_EXISTS)){
        g_print(CYAN_COLOR "[INFO] Pasta ja existe\n" RESET_COLOR);
        g_print(GREEN_COLOR "[COMMAND] Examinando folder..\n" RESET_COLOR);
        system("sh src/sh/brokenlinks.sh");
    } else {
        mkdir(SYM_AUDIO_DIR, 0777);
        g_print(GREEN_COLOR "[COMMAND] Pasta criada\n" RESET_COLOR);
    }
    create_music_list(SYM_AUDIO_DIR, widgets_data, music_holder_grid, play_selected_music);

    const gchar *home_dir = g_get_home_dir();
    g_object_set_data_full(G_OBJECT(window), "home_dir", (gpointer)home_dir, g_free);
    g_signal_connect(GTK_WIDGET(widgets_data->music_button), "clicked", G_CALLBACK(pause_audio), NULL);
    // g_signal_connect(GTK_WIDGET(widgets_data->progress_bar), "value-changed", G_CALLBACK(on_value_changed_music), widgets_data);

    g_object_unref(provider);
    g_object_unref(css_file);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), widgets_data);
    
    
    gtk_widget_set_visible(window, TRUE);
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    if(current_task) {
        g_cancellable_cancel(current_cancellable);
        g_object_unref(current_cancellable);
        g_object_unref(current_task);
    }
    g_object_unref(app);
    return status;
}
