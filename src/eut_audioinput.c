#include "eut_audioinput.h"
#include "eut_async.h"
#include "eut_logs.h"
#include "eut_utils.h"
#include "gio/gio.h"
#include "glib.h"
#include "time.h"
#include <gtk/gtk.h>
#include "eut_musiclistfunc.h"


static gboolean 
update_range_value(gpointer data) {
    SharedUIState *ui = data;

    g_mutex_lock(&ui->mutex);
    gfloat val = ui->current_peak;
    ui->pending_ui_update = FALSE;
    g_mutex_unlock(&ui->mutex);

    gtk_range_set_value(GTK_RANGE(ui->range), val);
    return G_SOURCE_REMOVE;
}

void 
cleanup_audio_input_data(EuteranInputAudio *user_data)
{
    log_info("Estou sendo chamado em %s", __func__);

    if(!user_data){
        log_error("Erro: contexto ou dados inválidos.");
        return;
    }

    if(user_data->loop){
        if(!user_data->loop_stopped){
            user_data->loop_stopped = TRUE;
            pw_main_loop_quit(user_data->loop);
        }
        pw_main_loop_destroy(user_data->loop);
    }
    if(user_data->stream){
        user_data->stream = nullptr;
    }

    if (user_data->wav_writer) {
        drwav_uninit(user_data->wav_writer);

        user_data->wav_writer = nullptr;
        

        if (user_data->recording_path) {
            add_music_to_list(
                user_data->ctx->main_object_ref,
                user_data->recording_path
            );
        }
        
    }

    if(user_data->ctx){
        log_info("cleaning ctx");
        user_data->ctx->type = OP_NONE;
        user_data->ctx->user_data = nullptr;
        user_data->ctx->cancellable = nullptr;
        g_free(user_data->ctx);
    }

    

    if(user_data->shared_ui){
        g_free(user_data->shared_ui);
    }

    g_free(user_data);
}

static void on_process(void *userdata)
{
        EuteranInputAudio *data = userdata;
        struct pw_buffer *b;
        struct spa_buffer *buf;
        float *samples, max;
        uint32_t c, n, n_channels, n_samples, peak;

        if(!data){
            data->loop_stopped = TRUE;
            log_info("Data nulo em on_process, encerrando loop.");
            return;
        }
        
        if(!data->loop){
            data->loop_stopped = TRUE;
            log_info("Loop ou stream nulo em on_process, encerrando loop.");
            return;
        }

        if (data->loop_stopped) {
            log_info("Loop parado em on_process, encerrando loop.");
            if (data->loop) pw_main_loop_quit(data->loop);
            return;
        }

        if ((b = pw_stream_dequeue_buffer(data->stream)) == nullptr) {
                pw_log_warn("out of buffers: %m");
                return;
        }
        buf = b->buffer;
        if ((samples = buf->datas[0].data) == nullptr){
            log_info("Samples nulo em on_process, encerrando loop.");
        }

        
        int16_t pcm_out[8192];
        uint32_t out_index = 0;
                
        n_channels = data->format.info.raw.channels;
        n_samples = buf->datas[0].chunk->size / sizeof(float);

        for (c = 0; c < data->format.info.raw.channels; c++) {
            max = 0.0f;
            for (n = c; n < n_samples; n += n_channels)
                    max = fmaxf(max, fabsf(samples[n]));
            
        }

        for (uint32_t i = 0; i < n_samples && out_index < 8192; ++i) {
            float s = samples[i];
            s = fmaxf(-1.0f, fminf(1.0f, s)); 
            pcm_out[out_index++] = (int16_t)(s * 32767.0f);
        }

        if (data->wav_writer) drwav_write_pcm_frames(data->wav_writer, out_index / n_channels, pcm_out);
        

        SharedUIState *ui = data->shared_ui;

        g_mutex_lock(&ui->mutex);
        ui->current_peak = max;

        if (!ui->pending_ui_update) {
            ui->pending_ui_update = TRUE;
            g_idle_add(update_range_value, ui);
        }
        g_mutex_unlock(&ui->mutex);


        pw_stream_queue_buffer(data->stream, b);
}


static void on_stream_param_changed(void *_data, uint32_t id, const struct spa_pod *param)
{
    EuteranInputAudio *data = _data;

    if (param == nullptr || id != SPA_PARAM_Format)
        return;

    if (spa_format_parse(param, &data->format.media_type, &data->format.media_subtype) < 0)
        return;

    if (data->format.media_type != SPA_MEDIA_TYPE_audio ||
        data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
        return;

    if (spa_format_audio_raw_parse(param, &data->format.info.raw) < 0)
        return;

    if (data->wav_writer == nullptr) {
        data->wav_writer = g_new0(drwav, 1);
        drwav_data_format format = {
            .container = drwav_container_riff,
            .format = DR_WAVE_FORMAT_PCM,
            .channels = data->format.info.raw.channels,
            .sampleRate = data->format.info.raw.rate,
            .bitsPerSample = 16
        };
        time_t time_;
        time(&time_);
        struct tm *time = localtime(&time_);


        gchar *date = g_strdup_printf("recording(%04d-%02d-%02d|%02d:%02d:%02d).wav", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
        
        const gchar *platform_path = get_platform_music_path();
        GFile *dir = g_file_new_for_path(g_build_filename(platform_path, "EuteranRecordings", nullptr));
        if (!g_file_test(g_file_get_path(dir), G_FILE_TEST_EXISTS)) {
            g_file_make_directory(dir, nullptr, nullptr);
        }
        g_object_unref(dir);

        gchar *full_path = g_build_filename(platform_path, "EuteranRecordings", date, nullptr);
        if (!drwav_init_file_write(data->wav_writer, full_path, &format, nullptr)) {
            log_error("Erro ao iniciar gravação WAV.");
            g_free(data->wav_writer);
            g_free(date);
            data->wav_writer = nullptr;
        } else {
            log_info("Gravação WAV iniciada com %d canais a %d Hz",
                     format.channels, format.sampleRate);

            data->recording_path = full_path;
        }
        g_free(date);
    }
}

static void on_stream_state_changed(void *_data, enum pw_stream_state old, 
                                   enum pw_stream_state state, const char *error)
{
    EuteranInputAudio *data = _data;
    if (state == PW_STREAM_STATE_UNCONNECTED && data->ctx && data->ctx->task) {
        log_info("Estou sendo chamado em %s", __func__);
        g_task_return_boolean(data->ctx->task, TRUE);
    }
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .param_changed = on_stream_param_changed,
    .process = on_process,
    .state_changed = on_stream_state_changed, 
};

void start_audio_input_recording(EuteranInputAudio *user_data)
{
        if(!user_data){
            log_error("Erro: contexto ou dados inválidos.");
            return;
        }

       EuteranInputAudio *data = user_data;

        if(!data){
            log_error("Erro: contexto ou dados inválidos.");
            return;
        }

        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct pw_properties *props;
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        pw_init(nullptr, nullptr);

        data->loop = pw_main_loop_new(nullptr);
        if (!data->loop) {
                log_error("Error ao criar loop");
                return;
        }

        
        props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                        PW_KEY_MEDIA_CATEGORY, "Capture",
                        PW_KEY_MEDIA_ROLE, "Music",
                        nullptr);
        data->stream = pw_stream_new_simple(
                        pw_main_loop_get_loop(data->loop),
                        "audio-capture",
                        props,
                        &stream_events,
                        data);

        if(!data->stream) {
                log_error("Error ao criar stream");
                return;
        }

        params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
                        &SPA_AUDIO_INFO_RAW_INIT(
                                .format = SPA_AUDIO_FORMAT_F32));

        pw_stream_connect(data->stream,
                          PW_DIRECTION_INPUT,
                          PW_ID_ANY,
                          PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS |
                          PW_STREAM_FLAG_RT_PROCESS,
                          params, 1);
        log_info("Iniciando input loop.");

        

        pw_main_loop_run(data->loop);
}
