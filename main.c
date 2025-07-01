#include <gtk/gtk.h>
#include "adwaita.h"
#include "src/audio.h"
#include "src/e_logs.h"
#include "src/ewindows.h"
#include "src/e_widgets.h"
#include "src/widget_properties.h"
#include "src/e_commandw.h"
#include "src/widgets_devices.h"
#include "src/constants.h"

static void 
on_activate
(
    AdwApplication *app, 
    gpointer user_data
) 
{
    GtkBuilder *builder = gtk_builder_new_from_file("ux/euteran_main.ui");
    GtkWidget *window = BUILDER_GET(builder, "main_window");
    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));

    GFile *css_file = get_file_from_path();
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_file(provider, css_file);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *menu_button = BUILDER_GET(builder, "menu_button");

    g_object_set_data(G_OBJECT(window), "menu_button_popover", menu_button);

    GtkWidget *button_select_folder = BUILDER_GET(builder, "button_select_folder");
    GtkWidget *button_bindings = BUILDER_GET(builder, "button_bindings");
    GtkWidget *button_devices = BUILDER_GET(builder, "button_devices");

    g_signal_connect(button_select_folder, "clicked", G_CALLBACK(select_folder),window);
    g_signal_connect(button_devices, "clicked", G_CALLBACK(construct_widget), window);

    WidgetsData *widgets_data = g_new0(WidgetsData, 1);
    if (!widgets_data) {
        printf("Erro: falha ao alocar WidgetsData\n");
        g_object_unref(provider);
        g_object_unref(css_file);
        return;
    }

    AUTO_INSERT(widgets_data->widgets_list);

    gtk_range_set_value(GTK_RANGE(GET_WIDGET(widgets_data->widgets_list, VOLUME_SLIDER)), last_volume);
    GtkWidget *music_holder_box = BUILDER_GET(builder, "music_holder_box");

    GtkDropTarget *target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
    gtk_drop_target_set_gtypes (target, (GType[1]) { GDK_TYPE_FILE_LIST, }, 1);
    g_signal_connect(target, "drop", G_CALLBACK (on_drop), widgets_data);
    gtk_widget_add_controller (GTK_WIDGET (window), GTK_EVENT_CONTROLLER (target));

    g_object_set_data(G_OBJECT(GET_WIDGET(widgets_data->widgets_list, WINDOW_PARENT)), "play_selected_music", (gpointer)play_selected_music);
    g_object_set_data(G_OBJECT(GET_WIDGET(widgets_data->widgets_list, WINDOW_PARENT)), "widgets_data", widgets_data);
    g_object_set_data(G_OBJECT(GET_WIDGET(widgets_data->widgets_list, WINDOW_PARENT)), "music_holder", music_holder_box);


    if (g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_EXISTS)) {
        log_message("folder já existe");
    } else {
        g_mkdir_with_parents(SYM_AUDIO_DIR, 0777);
        log_command("Pasta criada com sucesso");
    }
    create_music_list(SYM_AUDIO_DIR, widgets_data, music_holder_box, play_selected_music);

    g_signal_connect(GET_WIDGET(widgets_data->widgets_list, PROGRESS_BAR), "change-value", G_CALLBACK(on_clicked_progress_bar), widgets_data);
    g_signal_connect(GET_WIDGET(widgets_data->widgets_list, VOLUME_SLIDER), "value-changed", G_CALLBACK(on_volume_changed), widgets_data);
    g_signal_connect(GET_WIDGET(widgets_data->widgets_list, MUSIC_BUTTON), "clicked", G_CALLBACK(pause_audio), widgets_data);
    g_signal_connect(GET_WIDGET(widgets_data->widgets_list, WINDOW_PARENT), "destroy", G_CALLBACK(on_window_destroy), widgets_data);
    g_object_unref(provider);
    g_object_unref(css_file);


    monitor_audio_dir_linkfiles(SYM_AUDIO_DIR, widgets_data);
    monitor_audio_dir(SYM_AUDIO_DIR, widgets_data);

    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
}

int main(int argc, char *argv[]) {
    last_volume = get_volume_from_settings();
    adw_init();
    AdwApplication *app = adw_application_new("com.deyvis2k.Euteran", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);

     if(current_task) {
        g_cancellable_cancel(current_cancellable);
        g_object_unref(current_cancellable);
        g_object_unref(current_task);
    }

    g_object_unref(app);

    return status;
}
