#ifndef UTILS_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#define UTILS_H

#define MINUTE_CONVERT 1.0f / 60.0f


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
    GtkWidget *duration_box;
    double music_duration;
    double elapsed_time;
} WidgetsData;



const double *seconds_to_minute(double music_duration);
music_list_t list_files_musics(const char* dir);
char* cast_double_to_string(double value);
GFile *get_file_from_path();
double get_duration_from_file(const char *file);
void save_path_data(const gchar *path);

#endif
