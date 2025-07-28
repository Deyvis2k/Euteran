#include "eut_audiolinux.h"
#include "ogg/stb_vorbis.h"
#include "pipewire/keys.h"
#include "pipewire/stream.h"
#include <mpg123.h>
#include <spa/param/audio/format-utils.h>
#include "eut_logs.h"
#include "eut_widgetsfunctions.h"
#include "eut_settings.h"

GetDurationFunc get_duration_functions[MAX_MUSIC_TYPES] = {get_duration_mp3, get_duration_ogg, get_duration_wav};
SetupAudioFunc setup_audio_functions[MAX_MUSIC_TYPES] = {setup_audio_mp3, setup_audio_ogg, setup_audio_wav};

static inline void 
apply_volume(
    int16_t *buffer, 
    size_t  size
)
{
    for(int i = 0; i < size; i++){
        buffer[i] = (int16_t)(buffer[i] * euteran_settings_get_last_volume(euteran_settings_get()));
    }
}

static gboolean
cancel_progress_bar_timer(
    gpointer user_data
)
{
    if (progress_timer_id != 0) {
        g_source_remove(progress_timer_id);
        progress_timer_id = 0;
    }
    return G_SOURCE_REMOVE;
}


void search_on_audio(EuteranMainAudio *data){
    if (!data || !data->task_data)
        return;


    if(data->task_data->audio_type == MP3){
        off_t target = data->task_data->frameoff;
        off_t actual = mpg123_seek(data->mpg, target, SEEK_SET);
        if (actual < 0) {
            log_error("Erro ao buscar no MP3: %s", mpg123_strerror(data->mpg));
        } 
    } else if(data->task_data->audio_type == OGG){
        if(data->vorbis_valid && data->task_data->frameoff >= 0){
            stb_vorbis_seek_frame(data->vorbis, data->task_data->frameoff);
        } else {
            log_error("vorbis inválido no seek");
        }
    }  else if(data->task_data->audio_type == WAV){
        if(data->wav && data->task_data->frameoff >= 0){
            drwav_seek_to_pcm_frame(data->wav, data->task_data->frameoff);
        }
    }
}

void 
cleanup_process(void *data_) {
    EuteranMainAudio *data = data_;

    if(!data) {
        return;
    }

    data->valid = FALSE;
    if(data->task_data && progress_timer_id != 0) {
        g_idle_add_full(G_PRIORITY_DEFAULT, (GSourceFunc)cancel_progress_bar_timer, NULL, NULL);
    }

    if (data->stream) {
        pw_stream_set_active(data->stream, false);
        pw_stream_disconnect(data->stream);
        pw_stream_destroy(data->stream);
        data->stream = NULL;
    }
    if (data->loop) {
        if (!data->loop_stopped) {
            pw_main_loop_quit(data->loop);
        }
        pw_main_loop_destroy(data->loop);
    }
    if (data->task_data &&
            data->task_data->audio_type == OGG && data->vorbis) {
        stb_vorbis_close(data->vorbis);
        data->vorbis = NULL;
        data->vorbis_valid = FALSE;
    } else if (data->mpg) {
        mpg123_close(data->mpg);
        mpg123_delete(data->mpg);
        mpg123_exit();
    }
    if(data->wav) {
        drwav_free(data->wav, NULL);
        data->wav = NULL;
    }
    
    if (data->task_data) g_free(data->task_data);
    
    data->loop = NULL;
    data->task_data = NULL;
    log_warning("Processo de áudio encerrado em cleanup_process");
}


static void on_process(void *userdata) {
    EuteranMainAudio *data = userdata;

    struct pw_buffer *b;
    struct spa_buffer *buf;
    int16_t *dst;
    size_t bytes_to_copy;

    if (!data || !data->valid || !data->stream) {
        return;
    }

    if (!data->cancellable) {
        log_info("Cancellable nulo em on_process, encerrando loop.");
        data->loop_stopped = TRUE;
        if (data->loop) pw_main_loop_quit(data->loop);
        return;
    }

    if(data->loop_stopped){
        if (data->loop) pw_main_loop_quit(data->loop);
        return;
    }

        
    g_mutex_lock(&data->paused_mutex);
    if (data->paused == TRUE){
        g_mutex_unlock(&data->paused_mutex);
        return;
    } 
    g_mutex_unlock(&data->paused_mutex);
    

    if(!data->task_data){
        log_error("Invalid task data");
        data->loop_stopped = TRUE;
        pw_main_loop_quit(data->loop);
        return;
    }

    if (g_cancellable_is_cancelled(data->cancellable) && !data->loop_stopped) {
        log_info("Cancelando áudio, aguarde...");
        data->loop_stopped = TRUE;
        if(data->loop) pw_main_loop_quit(data->loop);
        return;
    }

    if(!data->stream){
        log_error("Invalid stream");
        data->loop_stopped = TRUE;
        data->stream = NULL;
        if(data->loop) pw_main_loop_quit(data->loop);
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
        if(!data->vorbis || !data->vorbis_valid){
            pw_stream_queue_buffer(data->stream, b);
            return;
        }
        int samples = stb_vorbis_get_frame_short_interleaved(data->vorbis, data->channels, dst, 4096);
        if (samples == 0) {
            log_info("Fim do áudio, aguarde...");
            data->loop_stopped = TRUE;
            pw_main_loop_quit(data->loop);
            pw_stream_queue_buffer(data->stream, b);
            return;
        }
        bytes_to_copy = samples * data->channels * sizeof(int16_t);
    } else if (data->task_data->audio_type == MP3) {
        int err;
        if (mpg123_read(data->mpg, (unsigned char *)dst, buf->datas[0].maxsize, &bytes_to_copy) == MPG123_DONE) {
            pw_log_info("Fim do áudio, aguarde...");
            data->loop_stopped = TRUE;
            pw_main_loop_quit(data->loop);
            pw_stream_queue_buffer(data->stream, b);
            return;
        }
        mpg123_getformat(data->mpg, &data->rate, &data->channels, &data->encoding);
    } else {
        if(!data->wav){
            data->loop_stopped = TRUE;
            pw_main_loop_quit(data->loop);
            pw_stream_queue_buffer(data->stream, b);
            return;
        }
        size_t frames_available = buf->datas[0].maxsize / (sizeof(int16_t) * data->wav->channels);
        drwav_uint64 frames_read = drwav_read_pcm_frames_s16(data->wav, frames_available, dst);
        if (frames_read == 0) {
            log_info("Fim do áudio, aguarde...");
            data->loop_stopped = TRUE;
            pw_main_loop_quit(data->loop);
            pw_stream_queue_buffer(data->stream, b);
            return;
        }
        bytes_to_copy = frames_read * data->wav->channels * sizeof(int16_t);
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

void play_audio(EuteranMainAudio *audio_passed) {
    if (!audio_passed) {
        log_error("Invalid parameters");
        return;
    }

    EuteranMainAudio *data = audio_passed;

    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    int err;


    data->valid = TRUE;

    g_mutex_lock(&data->paused_mutex);
    data->paused = FALSE;
    g_mutex_unlock(&data->paused_mutex);
    data->loop_stopped = FALSE;
    
    SimpleAudioData data_ = {
        .audio = data,
        .err = err
    };
    setup_based_on_type(&data_);

    pw_init(NULL, NULL);
    data->loop = pw_main_loop_new(NULL);
    if (!data->loop) {
        log_error("Erro ao criar o main loop");
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
        log_error("Erro ao criar o stream");
        data->loop = NULL;
        return;
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
        pw_stream_destroy(data->stream);
        data->stream = NULL;
        return;
    }

    if (data->cancellable && g_cancellable_is_cancelled(data->cancellable)) {
        log_info("Cancelado antes de tocar o áudio para data=%p", data);
        pw_stream_destroy(data->stream);
        data->stream = NULL;
        return;
    }

    
    pw_stream_set_active(data->stream, !(data->paused));

    pw_main_loop_run(data->loop);
}

double get_duration_mp3(const char *music_path) {
    mpg123_handle *mpg = NULL;
    int err;
    long rate;
    int channels, encoding;
    double duration = 0;

    mpg123_init();
    mpg = mpg123_new(NULL, &err);
    if (!mpg) {
        log_error("Falha ao criar handle mpg123: %s", mpg123_plain_strerror(err));
        return -1;
    }

    if (mpg123_open(mpg, music_path) != MPG123_OK) {
        log_error("Erro ao abrir o arquivo MP3: %s", mpg123_strerror(mpg));
        mpg123_delete(mpg);
       
        return -1;
    }

    mpg123_getformat(mpg, &rate, &channels, &encoding);
    const off_t frames = mpg123_length(mpg);

    if (frames > 0) {
        duration = (double)frames / rate;
    }

    mpg123_close(mpg);
    mpg123_delete(mpg);
    return duration;
}

double get_duration_ogg(const char *music_path) {
    if (!music_path || strlen(music_path) == 0) {
        log_error("Caminho do arquivo OGG inválido");
        return 0.0;
    }

    int error_ogg;
    double duration = 0.0;
#define GLIST_GET(type, list, index) ((type*)g_list_nth_data((list), (index)))
    stb_vorbis *f = stb_vorbis_open_filename(music_path, &error_ogg, NULL);
    if (!f) {
        log_error("Erro ao abrir o arquivo OGG: %d", error_ogg);
        return 0.0;
    }

    const stb_vorbis_info info = stb_vorbis_get_info(f);
    if (info.sample_rate <= 0) {
        log_error("Informações inválidas do arquivo OGG");
        stb_vorbis_close(f);
        return 0.0;
    }

    duration = stb_vorbis_stream_length_in_seconds(f);
    stb_vorbis_close(f);

    if (duration <= 0.0) {
        log_error("Duração inválida para o arquivo OGG: %f", duration);
        return 0.0;
    }

    return duration;
}

double get_duration_wav(const char *music_path) {
    drwav wav;
    drwav_init_file(&wav, music_path, NULL);
    //total 
    drwav_uint64 total;
    drwav_get_length_in_pcm_frames(&wav, &total);
    return (double)total / wav.sampleRate;
}


double get_duration(const char *music_path) {
    for (int i = 0; i < MAX_MUSIC_TYPES; i++) {
        if (g_str_has_suffix(music_path, audio_extensions[i])) {
            return get_duration_functions[i](music_path);
        }
    }
    return -1;
}

void setup_audio_mp3(SimpleAudioData *sdata){
    mpg123_init();  
    sdata->audio->mpg = mpg123_new(NULL, &sdata->err);
    if (!sdata->audio->mpg || mpg123_open(sdata->audio->mpg, sdata->audio->task_data->filename) != MPG123_OK) {
        log_error("Erro ao abrir o arquivo MP3: %s", sdata->audio->mpg ? mpg123_strerror(sdata->audio->mpg) : "mpg123_new falhou");
        if (sdata->audio->mpg) {
            mpg123_delete(sdata->audio->mpg);
        }
        mpg123_exit();
        g_free(sdata);
        return;
    }
    mpg123_getformat(sdata->audio->mpg, &sdata->audio->rate, &sdata->audio->channels, &sdata->audio->encoding);
    mpg123_format_none(sdata->audio->mpg);
    mpg123_format(sdata->audio->mpg, sdata->audio->rate, sdata->audio->channels, SPA_AUDIO_FORMAT_S16);
}

void setup_audio_ogg(SimpleAudioData *sdata){
    sdata->audio->vorbis = stb_vorbis_open_filename(sdata->audio->task_data->filename, &sdata->err, NULL);
    if (!sdata->audio->vorbis) {
        log_error("Erro ao abrir o arquivo OGG: %d", sdata->err);
        g_free(sdata);
        return;
    }
    sdata->audio->vorbis_valid = (sdata->audio->vorbis != NULL);
    const stb_vorbis_info info = stb_vorbis_get_info(sdata->audio->vorbis);
    if (info.sample_rate <= 0) {
        log_error("Informações inválidas do arquivo OGG");
        stb_vorbis_close(sdata->audio->vorbis);
        g_free(sdata);
        return;
    }
    sdata->audio->rate = info.sample_rate;
    sdata->audio->channels = info.channels;
    sdata->audio->encoding = SPA_AUDIO_FORMAT_S16;
}

void setup_audio_wav(SimpleAudioData *sdata){
    sdata->audio->wav = g_new0(drwav, 1);
    if(!sdata->audio->wav){
        log_error("Erro ao alocar WAV");
        g_free(sdata);
        return;
    }
    const drwav_bool32 success = drwav_init_file(sdata->audio->wav, sdata->audio->task_data->filename, NULL);
    if(success != DRWAV_TRUE){
        log_error("Erro ao abrir o arquivo WAV");
        g_free(sdata);
        return;
    }
    sdata->audio->rate = sdata->audio->wav->sampleRate;
    sdata->audio->channels = sdata->audio->wav->channels;
    sdata->audio->encoding = sdata->audio->wav->translatedFormatTag;
}

void setup_based_on_type(SimpleAudioData *audio_data){
    const EuteranMainAudio *task_data = audio_data->audio;
    for(int i = 0; i < MAX_MUSIC_TYPES; i++){
        if(g_str_has_suffix(task_data->task_data->filename, audio_extensions[i])){
            setup_audio_functions[i](audio_data);
            return;
        }
    }
}
