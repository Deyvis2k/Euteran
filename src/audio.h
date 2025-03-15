#ifndef AUDIO_H
 
#include "pipewire/main-loop.h"
#include <mpg123.h>  
#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>

#define DEFAULT_RATE      44100
#define DEFAULT_CHANNELS  2

static float last_volume = 0.5f;
static GMutex paused_mutex;
static gboolean paused;

typedef struct {
    float volume;
} volume_data;

struct data {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    mpg123_handle *mpg;
    unsigned char *buffer;
    size_t buffer_size;
    long rate;       
    int channels;    
    int encoding;
    volume_data *volume;
    GCancellable *cancellable;
    gboolean loop_stopped;
    gboolean *paused_ptr;
};






static void on_process(void *userdata);
static void apply_volume(int16_t *buffer, size_t size, float volume);
static const struct pw_stream_events stream_events;
double get_duration(const char *music_path);
void play_audio(const char *musicfile, volume_data *volume, GCancellable *cancellable, gboolean *paused);


#endif
