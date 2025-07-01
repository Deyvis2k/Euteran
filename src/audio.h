#pragma once

#include <mpg123.h>
#include <gtk/gtk.h>
#include "ogg/stb_vorbis.h"
#include "utils.h"
#include <pipewire/pipewire.h>

#define DEFAULT_RATE      44100
#define DEFAULT_CHANNELS  2

static gboolean paused;

static GMutex paused_mutex;
static gboolean paused;

extern float last_volume;

typedef enum{
    MP3,
    OGG
} AUDIO_TYPE;

typedef struct {
    char filename[1024];
    double music_duration;
    int64_t frameoff;
    AUDIO_TYPE audio_type;
    WidgetsData *widgets_data;

} AudioTaskData;

struct data {
    AudioTaskData *task_data;
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    GMutex mutex;
    gboolean vorbis_valid;
    mpg123_handle *mpg;
    unsigned char *buffer;
    size_t buffer_size;
    stb_vorbis *vorbis;
    long rate;
    int channels;
    int encoding;
    GCancellable *cancellable;
    gboolean loop_stopped;
    gboolean *paused_ptr;
};

static void on_process(void *userdata);
static void apply_volume(int16_t *buffer, size_t size);
static const struct pw_stream_events stream_events;
void play_audio(AudioTaskData *task_data, GCancellable *cancellable, gboolean *paused, WidgetsData *widgets_data);

void search_on_audio(struct data *data);
