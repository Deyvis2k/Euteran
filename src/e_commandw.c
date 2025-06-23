#include "e_commandw.h"
#include "e_logs.h"



static void subprocess_wrapper_callback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    SubprocessContext *ctx = (SubprocessContext *)user_data;

    if (ctx->callback)
        ctx->callback(G_SUBPROCESS(source_object), res, ctx->user_data);

    g_free(ctx);
}


void run_subprocess_async(const gchar *command_str,
                          SubprocessResultCallback callback,
                          gpointer user_data) {
    GError *error = NULL;
    GSubprocess *proc = g_subprocess_new(
        G_SUBPROCESS_FLAGS_NONE,
        &error,
        "/bin/sh", "-c", command_str, NULL
    );

    if (!proc) {
        log_error("Erro ao criar subprocesso: %s", error->message);
        g_error_free(error);
        return;
    }

    SubprocessContext *ctx = g_new(SubprocessContext, 1);
    ctx->callback = callback;
    ctx->user_data = user_data;

    g_subprocess_wait_async(proc, NULL, subprocess_wrapper_callback, ctx);
}
