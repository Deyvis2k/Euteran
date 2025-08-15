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

typedef enum {
    UX_FILE,
    STYLE_FILE,
} AbsolutePathTypeFile;

double *seconds_to_minute(double music_duration);
GList *list_files_musics(const char* dir);
char* cast_double_to_string(double value);
double string_to_double(const char *str);
void save_path_data(const gchar *path);
void trim(char *str);
char *get_within_quotes(const char *str);
void remove_if_not_number(char *str);
char *remove_outside_quotes(char *str);
char *cast_simple_double_to_string(double value);
double formatted_string_to_double(const char *str);
char *get_absolute_path(AbsolutePathTypeFile type);


const char *get_platform_music_path();
