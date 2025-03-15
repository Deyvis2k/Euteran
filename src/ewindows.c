#include "ewindows.h"
#include "gdk/gdk.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "gtk/gtkcssprovider.h"
#include "gtk/gtkshortcut.h"
#include "utils.h"
#include "e_widgets.h"
#include "constants.h"


void on_file_opened(GtkFileDialog *dialog, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;
    GFile *file = gtk_file_dialog_select_folder_finish(dialog, res, &error);
    
    g_assert(GTK_IS_WIDGET(user_data));
    GtkWidget *window = GTK_WIDGET(user_data);

    if (file) {
        gchar *filename = g_file_get_path(file);

        if (!g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_IS_DIR)) {
            g_print(RED_COLOR "Erro: diretório %s não existe, criando...\n" RESET_COLOR, SYM_AUDIO_DIR);
            g_mkdir_with_parents(SYM_AUDIO_DIR, 0755);
        }

        const gchar command[] = "ln -sf %s/*.mp3 %s";
        gchar *command_string = g_strdup_printf(command, filename, SYM_AUDIO_DIR);
        g_print(GREEN_COLOR "[COMMAND]: %s\n" RESET_COLOR, command_string);

        if (system(command_string) != 0) {
            g_print(RED_COLOR "Erro ao criar symlink\n" RESET_COLOR);
        }
        g_free(command_string);

        WidgetsData *widgets_data = g_object_get_data(G_OBJECT(window), "widgets_data");
        GtkWidget *grid = g_object_get_data(G_OBJECT(window), "grid_data");
        PlayMusicFunc play_selected_music = g_object_get_data(G_OBJECT(window), "play_selected_music");

        if (!widgets_data || !grid || !play_selected_music) {
            g_print(RED_COLOR "Erro: Dados inválidos em window\n" RESET_COLOR);
        } else {
            g_print("atualizando lista\n");
            create_music_list(SYM_AUDIO_DIR, widgets_data, grid, play_selected_music); // Use SYM_AUDIO_DIR
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


void select_file(GtkWidget *button, gpointer user_data) {

    GtkFileDialog *choose = gtk_file_dialog_new();
    GtkWidget *parent = GTK_WIDGET(user_data);
    g_assert(GTK_WIDGET(parent));

    gtk_file_dialog_select_folder(choose, 
                         GTK_WINDOW(parent), 
                         NULL, 
                         (GAsyncReadyCallback)on_file_opened, 
                         user_data);

}

static gboolean on_key_press(GtkEventControllerKey *controller, guint keyval,
    guint keycode,
    GdkModifierType state,
    gpointer user_data)
    {
    GtkWindow *window = GTK_WINDOW(user_data);
    

    if (keyval == GDK_KEY_Escape) {
        gtk_window_close(window);
        return TRUE;
    }

    return FALSE;
}


void create_new_window(GtkWidget *window_parent, gpointer user_data) {
    GtkWidget *window = gtk_window_new();
    GtkEventController *controller = gtk_event_controller_key_new();


    const gchar css[] = {
        ".select_file_window {"
        "background-color: rgba(48,48,48,0.5);"
        "border: 2px solid rgba(48,70,48,0.3);"
        "margin: 10px;"
        "}"
        ".button_class {"
        "background-color: rgb(220,138,120);"
        "margin: 10px;"
        "}"
        ".button_class:hover {"
        "background-color: rgba(219,137,119,0.8);"
        "margin: 10px;"
        "}"
        ".grid_class {"
        "background-color: rgba(48,48,48,0.5);"
        "border: 2px solid rgba(48,70,48,0.3);"
        "margin: 10px;"
        "}"
        ".label_class {"
        "font-weight: bold;"
        "font-family: JetbrainsMono Nerd Font;"
        "margin: 10px;"
        "}"
    };
    
    GtkCssProvider *css_ = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css_, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
    GTK_STYLE_PROVIDER(css_), GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_window_set_title(GTK_WINDOW(window), "Background");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_widget_add_css_class(GTK_WIDGET(window), "select_file_window");

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_widget_add_css_class(GTK_WIDGET(grid), "grid_class");

    gtk_window_set_child(GTK_WINDOW(window), grid);

    GtkWidget *box_button_folder = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_grid_attach(GTK_GRID(grid), box_button_folder, 0, 0, 1, 1);

    GtkWidget *label_folder = gtk_label_new("Escolha uma pasta que contenha apenas MP3");
    gtk_widget_add_css_class(GTK_WIDGET(label_folder), "label_class");
    gtk_box_append(GTK_BOX(box_button_folder), label_folder);
        
    GtkWidget *button = gtk_button_new_with_label("Open");
    gtk_widget_add_css_class(GTK_WIDGET(button), "button_class");
    gtk_box_append(GTK_BOX(box_button_folder), button);
    gtk_widget_set_cursor_from_name(button, "hand2");

    g_signal_connect(button, "clicked", G_CALLBACK(select_file), user_data);
    
    
    gtk_widget_add_controller(GTK_WIDGET(window), controller);
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_press), window);
    gtk_widget_set_visible(GTK_WIDGET(window),TRUE);
}


