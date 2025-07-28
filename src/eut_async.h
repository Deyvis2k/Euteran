#pragma once

#include <gtk/gtk.h>


typedef void (*SubprocessResultCallback)(GSubprocess *proc, GAsyncResult *res, gpointer user_data);

typedef struct {
    SubprocessResultCallback callback;
    gpointer user_data;
} SubprocessContext;

static void subprocess_wrapper_callback(GObject *source_object, GAsyncResult *res, gpointer user_data);
void run_subprocess_async(const gchar *command_str,
                          SubprocessResultCallback callback,
                          gpointer user_data);


void monitor_audio_dir(
    const gchar     *audio_dir,
    gpointer        user_data
);


typedef enum{
    OP_NONE,
    OP_INPUT_RECORDING,
    OP_OUTPUT_PLAY
} OperatinType;

typedef struct{
    GCancellable *cancellable;
    GTask        *task;
    OperatinType type;
    void         *main_object_ref;
    void         *user_data;
} AsyncOperationContext;


void
free_context(AsyncOperationContext *context);
