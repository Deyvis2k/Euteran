#include "eut_audioinput.h"
#include "eut_async.h"
#include "eut_logs.h"
#include "gio/gio.h"
#include <gtk/gtk.h>

static void on_process(void *userdata)
{
        AsyncOperationContext *context = userdata;
        if(!context){
            log_info("Contexto nulo em on_process, encerrando loop.");
            return;
        }

        EuteranInputAudio *data = g_task_get_task_data(context->task);
        struct pw_buffer *b;
        struct spa_buffer *buf;
        float *samples, max;
        uint32_t c, n, n_channels, n_samples, peak;


        if(!data){
            log_info("Data nulo em on_process, encerrando loop.");
            return;
        }
        
        if(!data->loop || !data->stream){
            return;
        }

        if(!context->cancellable){
            return;
        }

        if(g_cancellable_is_cancelled(context->cancellable)){
            log_info("Cancellable cancelado em on_process, encerrando loop.");
            if (data->loop) {
                pw_main_loop_quit(data->loop);
            }
            if (data->stream) data->stream = NULL;
            return;
        }


        if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
                pw_log_warn("out of buffers: %m");
                return;
        }
        buf = b->buffer;
        if ((samples = buf->datas[0].data) == NULL)
                return;

        n_channels = data->format.info.raw.channels;
        n_samples = buf->datas[0].chunk->size / sizeof(float);

        if(data->passed_data){
            for (c = 0; c < data->format.info.raw.channels; c++) {
                max = 0.0f;
                for (n = c; n < n_samples; n += n_channels)
                        max = fmaxf(max, fabsf(samples[n]));
                gtk_range_set_value(GTK_RANGE(data->passed_data), max);
            }
        }

        pw_stream_queue_buffer(data->stream, b);
}

static void
on_stream_param_changed(void *_data, uint32_t id, const struct spa_pod *param)
{
        EuteranInputAudio *data = _data;

        /* NULL means to clear the format */
        if (param == NULL || id != SPA_PARAM_Format)
                return;

        if (spa_format_parse(param, &data->format.media_type, &data->format.media_subtype) < 0)
                return;

        /* only accept raw audio */
        if (data->format.media_type != SPA_MEDIA_TYPE_audio ||
            data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
                return;

        /* call a helper function to parse the format for us. */
        spa_format_audio_raw_parse(param, &data->format.info.raw);

        fprintf(stdout, "capturing rate:%d channels:%d\n",
                        data->format.info.raw.rate, data->format.info.raw.channels);

}

static const struct pw_stream_events stream_events = {
        PW_VERSION_STREAM_EVENTS,
        .param_changed = on_stream_param_changed,
        .process = on_process,
};

static void do_quit(void *userdata, int signal_number)
{
        log_info("Sinal %d recebido, encerrando loop.", signal_number);
        EuteranInputAudio *data = userdata;
        pw_main_loop_quit(data->loop);
}

void start_audio_input_recording(AsyncOperationContext *op, EuteranInputAudio *initialize_audio)
{
        if(!initialize_audio){
            log_error("Error, no initialize_audio");
            return;
        }

       EuteranInputAudio *data = initialize_audio;

        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct pw_properties *props;
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        pw_init(NULL, NULL);

        data->loop = pw_main_loop_new(NULL);

        pw_loop_add_signal(pw_main_loop_get_loop(data->loop), SIGINT, do_quit, data);
        pw_loop_add_signal(pw_main_loop_get_loop(data->loop), SIGTERM, do_quit, data);

        props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                        PW_KEY_MEDIA_CATEGORY, "Capture",
                        PW_KEY_MEDIA_ROLE, "Music",
                        NULL);
        data->stream = pw_stream_new_simple(
                        pw_main_loop_get_loop(data->loop),
                        "audio-capture",
                        props,
                        &stream_events,
                        op);

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

        pw_main_loop_run(data->loop);

}

