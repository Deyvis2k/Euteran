#include "audio.h"
#include "glib-object.h"
#include "ogg/stb_vorbis.h"
#include "pipewire/keys.h"
#include "pipewire/stream.h"
#include <mpg123.h>
#include <spa/param/audio/format-utils.h>
#include "e_logs.h"
#include "utils.h"

static GMutex paused_mutex;
static gboolean paused = FALSE;

float last_volume = 0;

static void apply_volume(int16_t *buffer, size_t size){
    for(int i = 0; i < size; i++){
        buffer[i] = (int16_t)(buffer[i] * last_volume);
    }
}


void search_on_audio(struct data *data){
    if (!data || !data->task_data)
        return;

    g_mutex_lock(&data->mutex);

    if(data->task_data->audio_type == MP3){
        off_t target = data->task_data->frameoff;
        off_t actual = mpg123_seek(data->mpg, target, SEEK_SET);
        if (actual < 0) {
            log_error("Erro ao buscar no MP3: %s", mpg123_strerror(data->mpg));
        } 
    } else if(data->task_data->audio_type == OGG){
        if(data->vorbis_valid && data->task_data->frameoff >= 0){
            stb_vorbis_seek(data->vorbis, data->task_data->frameoff);
        } else {
            log_error("vorbis inválido no seek");
        }
    }

    g_mutex_unlock(&data->mutex);
}



static void on_process(void *userdata) {
    struct data *data = userdata;

    if (!data) {
        log_error("Invalid data pointer");
        return;
    }

    struct pw_buffer *b;
    struct spa_buffer *buf;
    int16_t *dst;
    size_t bytes_to_copy;

    g_mutex_lock(&paused_mutex);
    if (*data->paused_ptr == TRUE){
        g_mutex_unlock(&paused_mutex);
        return;
    } 
    g_mutex_unlock(&paused_mutex);
    
    if (g_cancellable_is_cancelled(data->cancellable) && !data->loop_stopped) {
        log_info("Cancelando áudio, aguarde...");
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
        printf("no data in buffer\n");
        pw_stream_queue_buffer(data->stream, b);
        return;
    }

    if (data->task_data->audio_type == OGG) {
        int samples = stb_vorbis_get_frame_short_interleaved(data->vorbis, data->channels, dst, 4096);
        if (samples == 0) {
            log_info("Fim do áudio, aguarde...");
            data->loop_stopped = TRUE;
            pw_main_loop_quit(data->loop);
            pw_stream_queue_buffer(data->stream, b);
            return;
        }
        bytes_to_copy = samples * data->channels * sizeof(int16_t);
    } else {
        int err;
        if (mpg123_read(data->mpg, (unsigned char *)dst, buf->datas[0].maxsize, &bytes_to_copy) == MPG123_DONE) {
            pw_log_info("Fim do áudio, aguarde...");
            data->loop_stopped = TRUE;
            pw_main_loop_quit(data->loop);
            pw_stream_queue_buffer(data->stream, b);
            return;
        }
        mpg123_getformat(data->mpg, &data->rate, &data->channels, &data->encoding);
    }
    apply_volume(dst, bytes_to_copy / sizeof(int16_t));

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = sizeof(int16_t) * data->channels;
    buf->datas[0].chunk->size = bytes_to_copy;

    pw_stream_queue_buffer(data->stream, b);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

void play_audio(AudioTaskData *task_data, GCancellable *cancellable, gboolean *paused, WidgetsData *widgets_data) {
    struct data *data = g_new0(struct data, 1);
    g_mutex_init(&data->mutex);

    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    int err;
    
    data->loop_stopped = FALSE;
    data->task_data = task_data;
    data->paused_ptr = paused;
    data->cancellable = cancellable;

    if (data->task_data->audio_type == OGG) {
        data->vorbis = stb_vorbis_open_filename(task_data->filename, &err, NULL);
        data->vorbis_valid = (data->vorbis != NULL);
        if (!data->vorbis) {
            printf("Erro ao abrir o arquivo OGG: %d\n", err);
            return;
        }
        stb_vorbis_info info = stb_vorbis_get_info(data->vorbis);
        data->rate = info.sample_rate;
        data->channels = info.channels;
        data->encoding = SPA_AUDIO_FORMAT_S16;
    } else {
        mpg123_init();  
        data->mpg = mpg123_new(NULL, &err);
        if (!data->mpg || mpg123_open(data->mpg, task_data->filename) != MPG123_OK) {
            log_error("Erro ao abrir o arquivo MP3: %s", data->mpg ? mpg123_strerror(data->mpg) : "mpg123_new falhou");
            if (data->mpg) {
                mpg123_delete(data->mpg);
            }
            mpg123_exit();
            return;
        }
        mpg123_getformat(data->mpg, &data->rate, &data->channels, &data->encoding);
        mpg123_format_none(data->mpg);
        mpg123_format(data->mpg, data->rate, data->channels, SPA_AUDIO_FORMAT_S16);
    }

    pw_init(NULL, NULL);
    data->loop = pw_main_loop_new(NULL);
    if (!data->loop) {
        printf("Erro ao criar o main loop\n");
        goto cleanup;
    }
    
    data->stream = pw_stream_new_simple(
        pw_main_loop_get_loop(data->loop),
        "Euteran",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Music",
            NULL),
        &stream_events,
        data);

    if (!data->stream) {
        printf("Erro ao criar o stream\n");
        goto cleanup;
    }

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
        &SPA_AUDIO_INFO_RAW_INIT(
            .format = SPA_AUDIO_FORMAT_S16,
            .channels = data->channels,
            .rate = data->rate));

    if (pw_stream_connect(data->stream,
                          PW_DIRECTION_OUTPUT,
                          PW_ID_ANY,
                          PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS,
                          params, 1) != 0) {
        log_error("Erro ao conectar o stream");
        goto cleanup;
    }

    if (cancellable && g_cancellable_is_cancelled(cancellable)) {
        log_info("Cancelado antes de tocar o áudio");
        goto cleanup;
    }

    if(widgets_data){
        GtkWidget *parent = GET_WIDGET(widgets_data->widgets_list, WINDOW_PARENT);
        if(!parent || !GTK_IS_WINDOW(parent))
            printf("Erro ao pegar o widget da janela\n");
        g_object_set_data(
            G_OBJECT(parent),
            "audio_data",
            data
        );
    }


    pw_stream_set_active(data->stream, !(*paused));
    log_info("Tocando áudio...");
    
    pw_main_loop_run(data->loop);

cleanup:
    if (data->stream) {
        pw_stream_disconnect(data->stream);
        pw_stream_destroy(data->stream);
    }
    if (data->loop) {
        if (!data->loop_stopped) {
            pw_main_loop_quit(data->loop);
        }
        pw_main_loop_destroy(data->loop);
    }
    if (data->task_data->audio_type == OGG && data->vorbis) {
        g_mutex_lock(&data->mutex);
        stb_vorbis_close(data->vorbis);
        data->vorbis = NULL;
        data->vorbis_valid = FALSE;
        g_mutex_unlock(&data->mutex);
    } else if (data->mpg) {
        mpg123_close(data->mpg);
        mpg123_delete(data->mpg);
        mpg123_exit();
    }
}
