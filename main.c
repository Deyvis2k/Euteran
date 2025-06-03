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
#include "locale.h"

typedef struct {
    char filename[512];
    volume_data *volume;
    double music_duration;
} AudioTaskData;

static GTask *current_task = NULL;
static GCancellable *current_cancellable = NULL;
static guint progress_timer_id = 0;
static gboolean light_mode = FALSE;

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

static gboolean on_drop(
    GtkDropTarget *target,
    const GValue *value,
    double x,
    double y,
    gpointer user_data
) {
    g_print("valor recebido (gvalue): %s\n", G_VALUE_TYPE_NAME(value));
    
    if (G_VALUE_HOLDS(value, G_TYPE_FILE)) {
        GFile *file = g_value_get_object(value); 
        if (!file) {
            g_print("Erro: GFile inválido\n");
            return FALSE;
        }

        gchar *uri = g_file_get_uri(file);
        gchar *path = g_file_get_path(file);
        if (!path) {
            g_print("Erro ao obter caminho do arquivo: %s\n", uri);
            g_free(uri);
            return FALSE;
        }

        g_strchomp(uri);
        g_strchomp(path);
        g_print("URI limpo: %s\n", uri);
        g_print("Caminho limpo: %s\n", path);

        gboolean result = add_music_to_list(user_data, path, uri, file, play_selected_music);
        
        g_free(uri);
        g_free(path);
        return result;
    }
    else if(G_VALUE_HOLDS(value, G_TYPE_STRING)) {
        const gchar *uri = g_value_get_string(value);
        GFile *file = g_file_new_for_path(uri);
        gchar *path = g_file_get_path(file);
        if (!path) {
            g_print("Caminho do arquivo: %s\n", path);
            g_free(path);
            g_object_unref(file);
            return FALSE;
        }
        add_music_to_list(user_data, path, uri, file, play_selected_music);
        g_free(path);
        g_object_unref(file);
        return TRUE;
    }

    else if (G_VALUE_HOLDS(value, G_TYPE_ARRAY)) {
        g_print(GREEN_COLOR "[INFO] Arquivos arrastados\n" RESET_COLOR);
        GdkFileList *file_list = g_value_get_boxed(value);
        GSList *files = gdk_file_list_get_files(file_list);
        for (GSList *iter = files; iter; iter = iter->next) {
            const gchar *uri = g_file_get_uri(iter->data);
            GFile *file = iter->data;
            gchar *path = g_file_get_path(file);
            if (!path) {
                g_print("Caminho do arquivo: %s\n", path);
                g_free(path);
                g_object_unref(file);
                return FALSE;
            }

            add_music_to_list(user_data, path, uri, file, play_selected_music);
            g_object_unref(file);
            g_free(path);
        }

        g_slist_free(files);
        g_object_unref(file_list);
        return TRUE;
    } else {
        g_print("Tipo de dado não suportado\n");
        return FALSE;
    }
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

    g_print("Último volume de música é igual a: %.2f\n", last_volume);
    save_current_settings(last_volume);
}

gboolean is_session_a_wm(const char* session_name, const char* actual_session_name) {
    return strcmp(session_name, actual_session_name) == 0;   
} 

void on_activate(GtkApplication *app, gpointer user_data) {
    AdwApplicationWindow *window = ADW_APPLICATION_WINDOW(adw_application_window_new(app));
    gchar *title;
#ifdef __linux__
    #include <stdlib.h>
    const char* sessions_names[] = {"i3", "hyprland", "sway", NULL};
    const char *session_name = getenv("DESKTOP_SESSION");
    for (int i = 0; sessions_names[i]; i++) {
        if (is_session_a_wm(sessions_names[i], session_name)) {
            title = "Background";
            break;
        }
    }
    if (title == NULL) {
        title = "Euteran";
    }
#else
    title = "Euteran";
#endif
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400); 
    gtk_widget_add_css_class(GTK_WIDGET(window), "main_window_class");


    GFile *css_file = get_file_from_path();
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_file(provider, css_file);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    adw_application_window_set_content(window, main_box);

    AdwHeaderBar *header_bar = ADW_HEADER_BAR(adw_header_bar_new());
    gtk_widget_add_css_class(GTK_WIDGET(header_bar), "header_bar");
    gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(header_bar));

    GtkWidget *menu_button = gtk_menu_button_new();
    gtk_menu_button_set_label(GTK_MENU_BUTTON(menu_button), "Options");
    adw_header_bar_pack_start(header_bar, menu_button);

    GtkWidget *popover = gtk_popover_new();
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), popover);
    // gtk_widget_add_css_class(popover, "popover");

    GtkWidget *popover_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_popover_set_child(GTK_POPOVER(popover), popover_box);
    // gtk_widget_add_css_class(popover_box, "popover_box");

    const char* popover_names[] = {"Select Folder", "Bindings", "Devices", NULL};
    for (int i = 0; popover_names[i]; i++) {
        GtkWidget *button = gtk_button_new_with_label(popover_names[i]);
        gtk_widget_set_hexpand(button, FALSE);
        // gtk_widget_add_css_class(button, "popover_button");
        gtk_box_append(GTK_BOX(popover_box), button);

        if (strcmp(popover_names[i], "Select Folder") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(select_folder), window);
            gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
        } else if (strcmp(popover_names[i], "Devices") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(construct_widget), window);
            gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
        }
    }

    WidgetsData *widgets_data = g_new0(WidgetsData, 1);
    if (!widgets_data) {
        printf("Erro: falha ao alocar WidgetsData\n");
        g_object_unref(provider);
        g_object_unref(css_file);
        return;
    }

    GtkWidget *music_display_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(music_display_content, GTK_ALIGN_BASELINE_FILL);
    gtk_widget_add_css_class(music_display_content, "music_display_content_class");

    GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
    gtk_widget_add_css_class(slider, "slider_main");
    gtk_widget_set_hexpand(slider, TRUE);
    gtk_widget_set_halign(slider, GTK_ALIGN_FILL);
    gtk_box_append(GTK_BOX(music_display_content), slider);

    GtkWidget *progress_bar = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
    gtk_widget_add_css_class(progress_bar, "progress_bar_class");
    gtk_widget_set_hexpand(progress_bar, TRUE);
    gtk_box_append(GTK_BOX(music_display_content), progress_bar);

    GtkWidget *music_button = gtk_button_new_from_icon_name("media-playback-start");
    gtk_widget_add_css_class(music_button, "music_button_class");
    gtk_widget_set_cursor_from_name(music_button, "hand2");
    gtk_box_append(GTK_BOX(music_display_content), music_button);

    widgets_data->music_display_content = music_display_content;
    widgets_data->volume_slider = slider;
    widgets_data->progress_bar = progress_bar;
    widgets_data->music_button = music_button;
    widgets_data->window_parent = GTK_WIDGET(window);

    AdwClamp *clamp = ADW_CLAMP(adw_clamp_new());
    adw_clamp_set_child(clamp, music_display_content);
    gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(clamp));

    GtkWidget *music_holder_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(music_holder_box, TRUE);
    gtk_widget_set_vexpand(music_holder_box, TRUE);
    gtk_widget_add_css_class(music_holder_box, "music_holder_box");
    
    GtkDropTarget *target =
      gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);

      gtk_drop_target_set_gtypes (target, (GType [5]) {
        G_TYPE_STRING,
        G_TYPE_FILE,
        G_TYPE_ARRAY,
        GDK_TYPE_FILE_LIST,
        G_TYPE_BOXED
      }, 5);

    g_signal_connect(target, "drop", G_CALLBACK(on_drop), widgets_data);
    gtk_widget_add_controller(GTK_WIDGET(music_holder_box), GTK_EVENT_CONTROLLER(target));

   
    GtkWidget *tag_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    // gtk_widget_add_css_class(tag_box, "tag_box");
    GtkWidget *tag_label_name = gtk_label_new("Music Name");
    gtk_widget_add_css_class(tag_label_name, "tag_label_name");
    gtk_widget_set_hexpand(tag_label_name, TRUE);
    gtk_box_append(GTK_BOX(tag_box), tag_label_name);
    GtkWidget *tag_label_duration = gtk_label_new("Duration");
    gtk_widget_set_margin_end(tag_label_duration, 7);
    gtk_widget_add_css_class(tag_label_duration, "tag_label_duration");
    gtk_box_append(GTK_BOX(tag_box), tag_label_duration);
    gtk_box_append(GTK_BOX(music_holder_box), tag_box);

    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_BROWSE);
    gtk_widget_add_css_class(list_box, "list_box");
    widgets_data->list_box = list_box;
    gtk_box_append(GTK_BOX(music_holder_box), list_box);

    AdwPreferencesGroup *prefs_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(prefs_group, "Music Library");
    adw_preferences_group_add(prefs_group, music_holder_box);
    gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(prefs_group));

    // AdwBreakpoint *breakpoint = adw_breakpoint_new(
    //     adw_breakpoint_condition_parse("max-width: 600px")
    // );
    // adw_breakpoint_add_setter(breakpoint, G_OBJECT(clamp), "maximum-size", (GValue *)g_variant_new_int32(400));
    // adw_application_window_add_breakpoint(window, breakpoint);

    // Store data
    g_object_set_data(G_OBJECT(window), "play_selected_music", (gpointer)play_selected_music);
    g_object_set_data(G_OBJECT(window), "widgets_data", widgets_data);
    g_object_set_data(G_OBJECT(window), "music_holder", music_holder_box);


    if (g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_EXISTS)) {
        g_print(CYAN_COLOR "[INFO] Pasta ja existe\n" RESET_COLOR);
        g_print(GREEN_COLOR "[COMMAND] Examinando folder..\n" RESET_COLOR);
        system("sh src/sh/brokenlinks.sh");
    } else {
        g_mkdir_with_parents(SYM_AUDIO_DIR, 0777);
        g_print(GREEN_COLOR "[COMMAND] Pasta criada\n" RESET_COLOR);
    }
    create_music_list(SYM_AUDIO_DIR, widgets_data, music_holder_box, play_selected_music);

    g_signal_handlers_disconnect_by_func(widgets_data->list_box, G_CALLBACK(play_selected_music), widgets_data);
    g_signal_connect(widgets_data->list_box, "row-activated", G_CALLBACK(play_selected_music), widgets_data);

    g_signal_connect(widgets_data->music_button, "clicked", G_CALLBACK(pause_audio), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), widgets_data);

    g_object_unref(provider);
    g_object_unref(css_file);

    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
}


int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "en_US.UTF-8");
    setenv("GTK_THEME", "Adwaita:dark", 1);
    g_print("volume from settings: %.2f\n", get_volume_from_settings());
    last_volume = get_volume_from_settings();
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
