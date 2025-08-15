#include "eut_main_object.h"
#include "eut_logs.h"
#include "eut_widgetsfunctions.h"
#include "eut_async.h"
#include "eut_constants.h"
#include "adwaita.h"

static void 
on_activate
(
    AdwApplication *app, 
    gpointer user_data
) 
{
    EuteranMainObject *self = euteran_main_object_new();
    gtk_window_set_application(GTK_WINDOW(self), GTK_APPLICATION(app));

    if (!g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_EXISTS)) {
        g_mkdir_with_parents(SYM_AUDIO_DIR, 0777);
        log_command("Pasta criada com sucesso");
    }

    monitor_audio_dir_linkfiles(SYM_AUDIO_DIR, self);
    monitor_audio_dir(SYM_AUDIO_DIR, self);
}

int main(int argc, char *argv[]) {
    adw_init();
    AdwApplication *app = adw_application_new("Deyvis2k.Euteran", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
