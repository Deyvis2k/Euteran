#include "eut_foldercb.h"
#include "eut_logs.h"
#include "eut_main_object.h"
#include "eut_musiclistfunc.h"
#include "eut_constants.h"
#include "eut_async.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"

static void 
on_command_done(
    GSubprocess     *proc, 
    GAsyncResult    *res, 
    gpointer        user_data
) 
{
    GError *error = nullptr;
    if (!g_subprocess_wait_finish(proc, res, &error)) {
        log_error("Erro ao executar comando: %s", error->message);
        g_error_free(error);
        return;
    }
    void **passed_data = user_data;

    if (passed_data[0] == NULL || passed_data[1] == NULL) {
        return;
    } else log_message("Passed data: %p, %p", passed_data[0], passed_data[1]);
    
    
    gchar *path_dir = passed_data[1];

    if (path_dir == NULL) {
        return;
    }

    log_message("Path dir: %s", path_dir);

    EuteranMainObject *widgets_data = (EuteranMainObject *)passed_data[0];

    if(!EUTERAN_IS_MAIN_OBJECT(widgets_data)){
        log_error("Not euteran main object");
        return;
    }


    if (widgets_data) {
        create_music_list(path_dir, widgets_data);
        log_message("Músicas carregadas com sucesso");
    }
    
    g_free(path_dir);
    g_free(user_data);
}



void on_folder_open(
    GtkFileDialog   *dialog, 
    GAsyncResult    *res, 
    gpointer        user_data
) 
{
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_select_folder_finish(dialog, res, &error);
    EuteranMainObject *widgets_data = EUTERAN_MAIN_OBJECT(user_data);

    if(!EUTERAN_IS_MAIN_OBJECT(widgets_data)){
        log_error("Not euteran main object");
        return;
    }

    if (file) {
        gchar *path_dir = g_file_get_path(file);

        if (!g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_IS_DIR)) {
            log_command("Criando a pasta %s...", SYM_AUDIO_DIR);
            g_mkdir_with_parents(SYM_AUDIO_DIR, 0755);
        }
        char command_buf[1024];
        snprintf(command_buf, sizeof(command_buf), 
         "sh -c \"find %s -type f \\( -iname \\*.mp3 -o -iname \\*.ogg -o -iname \\*.wav \\) -exec ln -sf {} %s \\;\"",
         path_dir, SYM_AUDIO_DIR);
        
        void **passed_data = malloc(sizeof(void *) * 2);
        passed_data[0] = widgets_data;
        passed_data[1] = path_dir;

        run_subprocess_async(command_buf, on_command_done, passed_data);

        g_object_unref(file);
    } else {
        if (error) {
            log_error("Erro ao abrir pasta: %s", error->message);
            g_error_free(error);
        }
    }

    g_object_unref(dialog);
}

void select_folder(EuteranMainObject *wd, GtkButton *button) {
    GtkFileDialog *choose = gtk_file_dialog_new();
    g_object_ref(choose);
    gtk_file_dialog_select_folder(
                         choose, 
                         GTK_WINDOW(wd),
                         nullptr,
                         (GAsyncReadyCallback)on_folder_open, 
                         wd);
}
