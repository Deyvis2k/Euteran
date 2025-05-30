#include "ewindows.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "gtk/gtkshortcut.h"
#include "utils.h"
#include "e_widgets.h"
#include "constants.h"


void on_folder_open(GtkFileDialog *dialog, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;
    GFile *file = gtk_file_dialog_select_folder_finish(dialog, res, &error);
    GtkWidget *window = GTK_WIDGET(user_data);

    if(!GTK_IS_WIDGET(window) || window == NULL) {
        g_print("Erro: window nao eh um GtkWidget\n");
        return;
    }

    if (file) {
        gchar *filename = g_file_get_path(file);

        if (!g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_IS_DIR)) {
            g_print(RED_COLOR "Erro: diretório %s não existe, criando...\n" RESET_COLOR, SYM_AUDIO_DIR);
            g_mkdir_with_parents(SYM_AUDIO_DIR, 0755);
        }

        const gchar command[] = "sh -c \"find %s -type f \\( -iname \\*.mp3 -o -iname \\*.ogg \\) -exec ln -sf {} %s \\;\"";
        gchar *command_string = g_strdup_printf(command, filename, SYM_AUDIO_DIR);
        g_print(GREEN_COLOR "[COMMAND]: %s\n" RESET_COLOR, command_string);

        if (system(command_string) != 0) {
            g_print(RED_COLOR "Erro ao criar symlink\n" RESET_COLOR);
        }
        g_free(command_string);

        WidgetsData *widgets_data = g_object_get_data(G_OBJECT(window), "widgets_data");
        GtkWidget *music_holder = g_object_get_data(G_OBJECT(window), "music_holder");
        PlayMusicFunc play_selected_music = g_object_get_data(G_OBJECT(window), "play_selected_music");

        if (!widgets_data || !music_holder || !play_selected_music) {
            g_print(RED_COLOR "Erro: Dados inválidos em window\n" RESET_COLOR);
        } else {
            g_print("atualizando lista\n");
            create_music_list(SYM_AUDIO_DIR, widgets_data, music_holder, play_selected_music); 
            g_print("lista atualizada\n");
        }

        g_free(filename);
        g_object_unref(file);
    } else {
        if (error) {
            g_print("erro: %s\n", error->message);
            g_error_free(error);
        }
        g_print("arquivo nao escolhido\n");
    }

    g_object_unref(dialog);
}

void select_folder(GtkWidget *button, gpointer user_data) {
    printf("Abrindo dialog\n");
    GtkFileDialog *choose = gtk_file_dialog_new();
    g_object_ref(choose);
    GtkWidget *parent = GTK_WIDGET(user_data);
    if(!GTK_IS_WIDGET(parent) || parent == NULL) {
        g_print("Erro: parent nao eh um GtkWidget\n");
        return;
    }
    gtk_file_dialog_select_folder(
                         choose, 
                         GTK_WINDOW(parent), 
                         NULL, 
                         (GAsyncReadyCallback)on_folder_open, 
                         user_data);
}
