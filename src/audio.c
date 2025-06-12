#include "audio.h"
#include "ogg/stb_vorbis.h"
#include "pipewire/keys.h"
#include "pipewire/stream.h"
#include <mpg123.h>
#include <gio/gio.h>
#include "constants.h"

static GMutex paused_mutex;
static gboolean paused = FALSE;

static float last_volume = 0.0f;

float get_last_volume() {
    return last_volume;
}
void set_last_volume(float volume) {
    last_volume = volume;
}


static void apply_volume(int16_t *buffer, size_t size, float volume){
    for(int i = 0; i < size; i++){
        buffer[i] = (int16_t)(buffer[i] * volume);
    }
}

static void on_process(void *userdata) {
    struct data *data = userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;
    int16_t *dst;
    size_t bytes_to_copy;

    g_mutex_lock(&paused_mutex);
    if (*data->paused_ptr) {
        g_mutex_unlock(&paused_mutex);
        return;
    }
    g_mutex_unlock(&paused_mutex);

    if (g_cancellable_is_cancelled(data->cancellable) && !data->loop_stopped) {
        printf(YELLOW_COLOR "[INFO] Cancelando áudio, aguarde...\n" RESET_COLOR);
        data->loop_stopped = TRUE;
        pw_main_loop_quit(data->loop);
        return;
    }

    if (!(b = pw_stream_dequeue_buffer(data->stream))) {
        pw_log_warn("out of buffers: %m");
        return;
    }

    buf = b->buffer;
    if (!(dst = buf->datas[0].data)) {
        pw_stream_queue_buffer(data->stream, b);
        return;
    }

    if (data->is_ogg) {
        int samples = stb_vorbis_get_frame_short_interleaved(data->vorbis, data->channels, dst, 4096);
        if (samples == 0) {
            data->loop_stopped = TRUE;
            pw_main_loop_quit(data->loop);
            pw_stream_queue_buffer(data->stream, b);
            return;
        }
        bytes_to_copy = samples * data->channels * sizeof(int16_t);
    } else {
        int err;
        if (mpg123_read(data->mpg, (unsigned char *)dst, buf->datas[0].maxsize, &bytes_to_copy) == MPG123_DONE) {
            data->loop_stopped = TRUE;
            pw_main_loop_quit(data->loop);
            pw_stream_queue_buffer(data->stream, b);
            return;
        }
    }

    apply_volume(dst, bytes_to_copy / sizeof(int16_t), data->volume->volume);

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = sizeof(int16_t) * data->channels;
    buf->datas[0].chunk->size = bytes_to_copy;

    pw_stream_queue_buffer(data->stream, b);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

double get_duration(const char *music_path) {
    mpg123_handle *mpg;
    int err;
    long rate;
    int channels, encoding;
    off_t frames;
    double duration = 0;

    mpg123_init();
    mpg = mpg123_new(NULL, &err);
    if (!mpg) {
        return -1;
    }

    if (mpg123_open(mpg, music_path) != MPG123_OK) {
        mpg123_delete(mpg);
        mpg123_exit();
        return -1;
    }

    mpg123_getformat(mpg, &rate, &channels, &encoding);
    frames = mpg123_length(mpg);

    if (frames > 0) {
        duration = (double)frames / rate;
    }


    mpg123_close(mpg);
    mpg123_delete(mpg);
    mpg123_exit();
    return duration;
}

double get_duration_ogg(const char *music_path){
    int error;
    double duration = 0;
    stb_vorbis *f = stb_vorbis_open_filename(music_path, &error, NULL);
    if(!f){
        printf("Error: %d\n", error);
        return 0;
    }
    duration = stb_vorbis_stream_length_in_seconds(f);
    stb_vorbis_close(f);
    return duration;
}

void play_audio(const char *musicfile, volume_data *volume, GCancellable *cancellable, gboolean *paused, GtkWidget *parent) {
    struct data data = { 0 };
    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    int err;

    data.loop_stopped = FALSE;
    data.paused_ptr = paused;
    data.volume = volume;
    data.cancellable = cancellable;

    const char *ext = strrchr(musicfile, '.');
    data.is_ogg = ext && (strcmp(ext, ".ogg") == 0);

    if (data.is_ogg) {
        data.vorbis = stb_vorbis_open_filename(musicfile, &err, NULL);
        if (!data.vorbis) {
            printf("Erro ao abrir o arquivo OGG: %d\n", err);
            return;
        }
        stb_vorbis_info info = stb_vorbis_get_info(data.vorbis);
        data.rate = info.sample_rate;
        data.channels = info.channels;
        data.encoding = SPA_AUDIO_FORMAT_S16;
    } else {
        mpg123_init();
        data.mpg = mpg123_new(NULL, &err);
        if (!data.mpg || mpg123_open(data.mpg, musicfile) != MPG123_OK) {
            printf("Erro ao abrir o arquivo MP3: %s\n", data.mpg ? mpg123_strerror(data.mpg) : "mpg123_new falhou");
            if (data.mpg) {
                mpg123_delete(data.mpg);
            }
            mpg123_exit();
            return;
        }
        mpg123_getformat(data.mpg, &data.rate, &data.channels, &data.encoding);
        mpg123_format_none(data.mpg);
        mpg123_format(data.mpg, data.rate, data.channels, SPA_AUDIO_FORMAT_S16);
    }

    pw_init(NULL, NULL);
    data.loop = pw_main_loop_new(NULL);
    if (!data.loop) {
        printf("Erro ao criar o main loop\n");
        goto cleanup;
    }

    data.stream = pw_stream_new_simple(
        pw_main_loop_get_loop(data.loop),
        "Euteran",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Music",
            NULL),
        &stream_events,
        &data);

    if (!data.stream) {
        printf("Erro ao criar o stream\n");
        goto cleanup;
    }

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
        &SPA_AUDIO_INFO_RAW_INIT(
            .format = SPA_AUDIO_FORMAT_S16,
            .channels = data.channels,
            .rate = data.rate));

    if (pw_stream_connect(data.stream,
                          PW_DIRECTION_OUTPUT,
                          PW_ID_ANY,
                          PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS,
                          params, 1) != 0) {
        printf("Erro ao conectar o stream\n");
        goto cleanup;
    }

    if (cancellable && g_cancellable_is_cancelled(cancellable)) {
        printf("Cancelado antes de tocar o áudio\n");
        goto cleanup;
    }

    pw_stream_set_active(data.stream, !(*paused));
    printf(YELLOW_COLOR "[INFO] Tocando áudio...\n" RESET_COLOR);
    pw_main_loop_run(data.loop);

cleanup:
    if (data.stream) {
        pw_stream_disconnect(data.stream);
        pw_stream_destroy(data.stream);
    }
    if (data.loop) {
        if (!data.loop_stopped) {
            pw_main_loop_quit(data.loop);
        }
        pw_main_loop_destroy(data.loop);
    }
    if (data.is_ogg && data.vorbis) {
        stb_vorbis_close(data.vorbis);
    } else if (data.mpg) {
        mpg123_close(data.mpg);
        mpg123_delete(data.mpg);
        mpg123_exit();
    }
}
