#include "ewindows.h"
#include "e_logs.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "gtk/gtkshortcut.h"
#include "utils.h"
#include "e_widgets.h"
#include "constants.h"
#include "e_commandw.h"

static void 
on_command_done(
    GSubprocess     *proc, 
    GAsyncResult    *res, 
    gpointer        user_data
) 
{
    GError *error = NULL;
    if (!g_subprocess_wait_finish(proc, res, &error)) {
        log_error("Erro ao executar comando: %s", error->message);
        g_error_free(error);
        return;
    }

    GtkWidget *window = GTK_WIDGET(user_data);
    WidgetsData *widgets_data = g_object_get_data(G_OBJECT(window), "widgets_data");
    GtkWidget *music_holder = g_object_get_data(G_OBJECT(window), "music_holder");
    PlayMusicFunc play_selected_music = g_object_get_data(G_OBJECT(window), "play_selected_music");

    if (widgets_data && music_holder && play_selected_music) {
        create_music_list(SYM_AUDIO_DIR, widgets_data, music_holder, play_selected_music);
        log_message("Músicas carregadas com sucesso");
    }
}



void on_folder_open(
    GtkFileDialog   *dialog, 
    GAsyncResult    *res, 
    gpointer        user_data
) 
{
    GError *error = NULL;
    GFile *file = gtk_file_dialog_select_folder_finish(dialog, res, &error);
    GtkWidget *window = GTK_WIDGET(user_data);

    if(!GTK_IS_WIDGET(window) || window == NULL) {
        log_error("Erro: window nao eh um GtkWidget"); 
        return;
    }

    if (file) {
        gchar *filename = g_file_get_path(file);

        if (!g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_IS_DIR)) {
            log_command("Criando a pasta %s...", SYM_AUDIO_DIR);
            g_mkdir_with_parents(SYM_AUDIO_DIR, 0755);
        }

        const gchar command[] = "sh -c \"find %s -type f \\( -iname \\*.mp3 -o -iname \\*.ogg \\) -exec ln -sf {} %s \\;\"";
        gchar *command_string = g_strdup_printf(command, filename, SYM_AUDIO_DIR);
        run_subprocess_async(command_string, on_command_done, window);
        g_free(command_string);
        g_free(filename);
        g_object_unref(file);
    } else {
        if (error) {
            log_error("Erro ao abrir pasta: %s", error->message);
            g_error_free(error);
        }
    }

    g_object_unref(dialog);
}

void select_folder(GtkWidget *button, gpointer user_data) {

    GtkWidget *window_parent = (GtkWidget *)user_data;
    if(!GTK_IS_WIDGET(window_parent) || window_parent == NULL) {
        log_error("Erro: window nao eh um GtkWidget"); 
        return;
    }

    GtkWidget *menu_button_object = g_object_get_data(
    G_OBJECT(window_parent), "menu_button_popover");

    if(menu_button_object)
        gtk_menu_button_popdown(GTK_MENU_BUTTON(menu_button_object));

    GtkFileDialog *choose = gtk_file_dialog_new();
    g_object_ref(choose);
    gtk_file_dialog_select_folder(
                         choose, 
                         GTK_WINDOW(window_parent),
                         NULL,
                         (GAsyncReadyCallback)on_folder_open, 
                         user_data);
}
