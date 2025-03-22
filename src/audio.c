#include "audio.h"
#include "pipewire/keys.h"
#include "pipewire/stream.h"
#include <mpg123.h>
#include <gio/gio.h>
#include "constants.h"

static GMutex paused_mutex;
static gboolean paused = FALSE;

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
    int err;
    size_t bytes_to_copy;

    g_mutex_lock(&paused_mutex);
    if(*data->paused_ptr) {
        g_mutex_unlock(&paused_mutex);
        return;
    }
    g_mutex_unlock(&paused_mutex);

    if(g_cancellable_is_cancelled(data->cancellable) && !data->loop_stopped) {
        printf(YELLOW_COLOR"[INFO] Cancelando áudio, aguarde...\n"RESET_COLOR);
        data->loop_stopped = TRUE;
        pw_main_loop_quit(data->loop);
        return;
    }

    if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
        pw_log_warn("out of buffers: %m");
        return;
    }

    buf = b->buffer;
    if ((dst = buf->datas[0].data) == NULL)
        return;

    err = mpg123_read(data->mpg, (unsigned char *)dst, buf->datas[0].maxsize, &bytes_to_copy);

    if (err == MPG123_DONE || (g_cancellable_is_cancelled(data->cancellable) && !data->loop_stopped)) {
        data->loop_stopped = TRUE;
        pw_main_loop_quit(data->loop);
        return;
    }
    apply_volume(dst, bytes_to_copy / sizeof(int16_t), data->volume->volume);

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = sizeof(int16_t) * data->channels;  // Usa canais reais
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
    double duration = -1;

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

void play_audio(const char* musicfile, volume_data *volume, GCancellable *cancellable, gboolean *paused, GtkWidget *parent) {
    struct data data = { 0 };
    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    int err;
    
    data.loop_stopped = FALSE;

    data.paused_ptr = paused;
    
    mpg123_init();
    data.mpg = mpg123_new(NULL, &err);
    if (!data.mpg) {
        printf("Erro ao inicializar mpg123\n");
        return;
    }

    if (mpg123_open(data.mpg, musicfile) != MPG123_OK) {
        printf("Erro: %s\n", mpg123_strerror(data.mpg));
        mpg123_delete(data.mpg);
        mpg123_exit();
        return;
    }

    mpg123_getformat(data.mpg, &data.rate, &data.channels, &data.encoding);
    
    mpg123_format_none(data.mpg);
    mpg123_format(data.mpg, data.rate, data.channels, data.encoding);

    pw_init(NULL, NULL);
    data.loop = pw_main_loop_new(NULL);

    if(!data.loop){
        printf("Error while creating main loop\n");
        mpg123_close(data.mpg);
        mpg123_delete(data.mpg);
        mpg123_exit();
        return;
    }
    
    data.stream = pw_stream_new_simple(
        pw_main_loop_get_loop(data.loop),
        "Soundpad",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Music",
            PW_KEY_TARGET_OBJECT, "VirtualMicSink",
            NULL),
        &stream_events,
        &data);

    data.volume = volume;
    data.cancellable = cancellable;

    if(!data.stream){
        printf("Error while creating stream\n");
        goto cleanup;
    }
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
        &SPA_AUDIO_INFO_RAW_INIT(
            .format = SPA_AUDIO_FORMAT_S16,
            .channels = data.channels,  
            .rate = data.rate));        

    pw_stream_connect(data.stream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        PW_STREAM_FLAG_AUTOCONNECT |
        PW_STREAM_FLAG_MAP_BUFFERS |
        PW_STREAM_FLAG_RT_PROCESS,
        params, 1);
    
    if(cancellable && g_cancellable_is_cancelled(cancellable)) {
        printf("Canceled before playing audio\n");
        goto cleanup;
    }
    
    pw_stream_set_active(data.stream, !(*paused));
    printf(YELLOW_COLOR "[INFO] Playing audio...\n" RESET_COLOR);
    pw_main_loop_run(data.loop);

cleanup:
    if(data.stream){
        pw_stream_disconnect(data.stream);
        pw_stream_destroy(data.stream);
        data.stream = NULL;
    }
    if(data.loop){
        if(!data.loop_stopped){
            pw_main_loop_quit(data.loop);
            data.loop_stopped = TRUE;
        }
        pw_main_loop_destroy(data.loop);
        data.loop = NULL;
    }
    if(data.mpg){
        mpg123_close(data.mpg);
        mpg123_delete(data.mpg);
        data.mpg = NULL;
    }
    mpg123_exit();
}

