#pragma once

#include <gio/gio.h>
#include <gtk/gtk.h>

#define MINUTE_CONVERT 1.0f / 60.0f
#define IS_ALLOWED_EXTENSION(filename) (g_str_has_suffix(filename, ".mp3") || g_str_has_suffix(filename, ".ogg") || g_str_has_suffix(filename, ".wav"))

typedef void (*PlayMusicFunc)(GtkListBox*, GtkListBoxRow *, gpointer);

typedef struct{
    double duration;
    char* name;
} music_t;

typedef enum{
    MUSIC_DISPLAY_CONTENT,
    PROGRESS_BAR,
    VOLUME_SLIDER,
    MUSIC_BUTTON,
    LIST_BOX,
    WINDOW_PARENT,
    MENU_BUTTON,
    STACK,
    STACK_SWITCHER,
    SWITCHER_ADD_BUTTON,
    STOP_BUTTON,
    INPUT_SLIDER,
    INPUT_BUTTON,
    STOP_RECORDING_BUTTON,
    MAX_WIDGETS
} WIDGETS;

#define BUILDER_GET(builder, name) GTK_WIDGET(gtk_builder_get_object(builder, name))

#define APPEND_WIDGET(list, widget_name) \
    list = g_list_append(list, widget_name);

static const char *widgets_names[MAX_WIDGETS] = {
    "music_display_content",
    "progress_bar",
    "volume_slider",
    "music_button",
    "list_box",
    "main_window",
    "menu_button",
    "view_stack_main",
    "stack_switcher",
    "switcher_add_button",
    "stop_button",
    "input_slider",
    "button_input_recording",
    "button_input_stop_recording"

};

double *seconds_to_minute(double music_duration);
GList *list_files_musics(const char* dir);
char* cast_double_to_string(double value);
double string_to_double(const char *str);
GFile *get_file_from_path();
void save_path_data(const gchar *path);
void trim(char *str);
char *get_within_quotes(const char *str);
void remove_if_not_number(char *str);
char *remove_outside_quotes(char *str);
char *cast_simple_double_to_string(double value);
double formatted_string_to_double(const char *str);

