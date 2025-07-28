#pragma once

#include <mpg123.h>
#include <gtk/gtk.h>
#include "ogg/stb_vorbis.h"
#include "eut_constants.h"
#include <pipewire/pipewire.h>
#include "wav/dr_wav.h"

#define DEFAULT_RATE      44100
#define DEFAULT_CHANNELS  2

typedef double(*GetDurationFunc)(const char *music_path);

static const char *audio_extensions[MAX_MUSIC_TYPES] = {
    ".mp3", 
    ".ogg", 
    ".wav"
};

extern GetDurationFunc get_duration_functions[MAX_MUSIC_TYPES];

typedef struct {
    char filename[1024];
    double music_duration;
    int64_t frameoff;
    AUDIO_TYPE audio_type;
} EuteranAudioTaskData;


typedef struct {
    EuteranAudioTaskData *task_data;
    drwav *wav;

    struct pw_main_loop *loop;
    struct pw_stream *stream;
    gboolean vorbis_valid;
    mpg123_handle *mpg;
    unsigned char *buffer;
    size_t buffer_size;
    stb_vorbis *vorbis;
    long rate;
    int channels;
    int encoding;
    GCancellable *cancellable;
    GMutex paused_mutex;
    gboolean paused;
    gboolean valid;
    gboolean loop_stopped;
} EuteranMainAudio;

typedef struct{
    int err;
    EuteranMainAudio *audio;
} SimpleAudioData;

typedef void(*SetupAudioFunc)(SimpleAudioData *data);
extern SetupAudioFunc setup_audio_functions[MAX_MUSIC_TYPES];




static void on_process(void *userdata);
static void apply_volume(int16_t *buffer, size_t size);
static const struct pw_stream_events stream_events;
void play_audio(EuteranMainAudio *audio_passed);
void search_on_audio(EuteranMainAudio *data);
void cleanup_process(void *data);

double get_duration_ogg(const char *music_path);
double get_duration_mp3(const char *music_path);
double get_duration_wav(const char *music_path);

double get_duration(const char *music_path);


void setup_audio_ogg(SimpleAudioData *sdata);
void setup_audio_mp3(SimpleAudioData *sdata);
void setup_audio_wav(SimpleAudioData *sdata);
void setup_based_on_type(SimpleAudioData *audio_data);
