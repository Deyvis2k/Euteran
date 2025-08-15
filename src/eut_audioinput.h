#pragma once

#include <spa/param/audio/format-utils.h>
#include "eut_async.h"
#include "gio/gio.h"
#include "wav/dr_wav.h"
#include <pipewire/pipewire.h>

extern guint input_slider_id;

typedef struct{
    GtkRange *range;
    gfloat current_peak;
    GMutex mutex;
    gboolean pending_ui_update;
} SharedUIState;


typedef struct{
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    struct spa_audio_info format;
    gboolean loop_stopped;
    SharedUIState *shared_ui;
    AsyncOperationContext *ctx;
    drwav *wav_writer;
    gchar *recording_path;
} EuteranInputAudio;


void
start_audio_input_recording(EuteranInputAudio *user_data);

void
cleanup_audio_input_data(EuteranInputAudio *user_data);
