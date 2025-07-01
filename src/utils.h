#pragma once

#include <gio/gio.h>
#include <gtk/gtk.h>


#define MINUTE_CONVERT 1.0f / 60.0f
#define ALLOWED_EXTENSION ".mp3"


typedef void (*PlayMusicFunc)(GtkListBox*, GtkListBoxRow *, gpointer);

typedef struct{
    double duration;
    char* name;
} music_t;

typedef struct {
    music_t *musics;
    size_t count_size;
} music_list_t;


typedef struct{
    GList *widgets_list;
    double music_duration;
    double elapsed_time;
} WidgetsData;

typedef enum{
    MUSIC_DISPLAY_CONTENT,
    PROGRESS_BAR,
    VOLUME_SLIDER,
    MUSIC_BUTTON,
    LIST_BOX,
    WINDOW_PARENT,
    MAX_WIDGETS
} WIDGETS;

#define BUILDER_GET(builder, name) GTK_WIDGET(gtk_builder_get_object(builder, name))
#define GET_WIDGET(list, index) ((GtkWidget*)g_list_nth_data((list), (WIDGETS)(index)))

#define APPEND_WIDGET(list, widget_name) \
    list = g_list_append(list, widget_name);

static const char *widgets_names[MAX_WIDGETS] = {
    "music_display_content",
    "progress_bar",
    "volume_slider",
    "music_button",
    "list_box",
    "main_window"
};

#define AUTO_INSERT(widgets_list) \
    for (int i = 0; i < MAX_WIDGETS; i++) { \
         GtkWidget *widget = BUILDER_GET(builder, widgets_names[i]); \
         if(!widget) { \
             printf("Widget %s not found\n", widgets_names[i]); \
             continue;\
         }\
         APPEND_WIDGET(widgets_list, widget); \
    }



const double *seconds_to_minute(double music_duration);
music_list_t list_files_musics(const char* dir);
char* cast_double_to_string(double value);
double string_to_double(const char *str);
GFile *get_file_from_path();
double get_duration_from_file(const char *file);
void save_path_data(const gchar *path);
void trim(char *str);
char *get_within_quotes(const char *str);
void remove_if_not_number(char *str);
char *remove_outside_quotes(char *str);
char *cast_simple_double_to_string(double value);

void save_current_settings(float last_volume);
float get_volume_from_settings();

double get_duration_ogg(const char *music_path);
double get_duration(const char *music_path);
