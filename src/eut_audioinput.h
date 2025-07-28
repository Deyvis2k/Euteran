#pragma once

#include <spa/param/audio/format-utils.h>
#include "eut_async.h"
#include "gio/gio.h"
#include "wav/dr_wav.h"
#include <pipewire/pipewire.h>

extern guint input_slider_id;


typedef struct{
    struct pw_main_loop *loop;
    struct pw_stream *stream;

    struct spa_audio_info format;

    void *passed_data;
} EuteranInputAudio;


void
start_audio_input_recording(AsyncOperationContext *context, EuteranInputAudio *initialize_audio);

