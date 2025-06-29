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
    GtkWidget *music_display_content;
    GtkWidget *progress_bar;
    GtkWidget *volume_slider;
    GtkWidget *music_button;
    GtkWidget *list_box;
    GtkWidget *window_parent;
    double music_duration;
    double elapsed_time;
    GtkShortcutController *controlador_key;
} WidgetsData;



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
