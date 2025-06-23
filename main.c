#include <gtk/gtk.h>
#include "audio.h"
#include "e_logs.h"
#include "src/ewindows.h"
#include "src/e_widgets.h"
#include "locale.h"
#include "src/widget_properties.h"
#include "adwaita.h"
#include "src/widgets_devices.h"
#include "src/utils.h"
#include "constants.h"
#include "src/e_commandw.h"

static void 
on_activate
(
    AdwApplication *app, 
    gpointer user_data
) 
{
    GtkWidget *window = adw_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "Euteran");
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_widget_add_css_class(GTK_WIDGET(window), "main_window_class");


    GFile *css_file = get_file_from_path();
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_file(provider, css_file);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(main_box, "main_box");
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), main_box);

    AdwHeaderBar *header_bar = ADW_HEADER_BAR(adw_header_bar_new());
    gtk_widget_add_css_class(GTK_WIDGET(header_bar), "header_bar");
    adw_header_bar_set_show_title(header_bar, FALSE);
    adw_header_bar_set_show_back_button(header_bar, FALSE);
    gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(header_bar));

    GtkWidget *menu_button = gtk_menu_button_new();
    gtk_widget_add_css_class(GTK_WIDGET(menu_button), "menu_button");
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "document-properties-symbolic");
    gtk_menu_button_set_can_shrink(GTK_MENU_BUTTON(menu_button), FALSE);
    adw_header_bar_pack_start(header_bar, menu_button);

    GtkWidget *popover = gtk_popover_new();
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
    gtk_widget_set_halign(GTK_WIDGET(popover), GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(GTK_WIDGET(popover), 30, 30);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), popover);
    // gtk_widget_add_css_class(popover, "popover");

    GtkWidget *popover_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_popover_set_child(GTK_POPOVER(popover), popover_box);
    // gtk_widget_add_css_class(popover_box, "popover_box");

    g_object_set_data(G_OBJECT(window), "menu_button_popover", menu_button);

    const char* popover_names[] = {"Select Folder", "Bindings", "Devices", NULL};
    for (int i = 0; popover_names[i]; i++) {
        GtkWidget *button = gtk_button_new_with_label(popover_names[i]);
        gtk_widget_set_hexpand(button, FALSE);
        // gtk_widget_add_css_class(button, "popover_button");
        gtk_box_append(GTK_BOX(popover_box), button);

        if (strcmp(popover_names[i], "Select Folder") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(select_folder),window);
        } else if (strcmp(popover_names[i], "Devices") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(construct_widget), window);
        } else if (strcmp(popover_names[i], "Bindings") == 0){
            gtk_popover_popdown(GTK_POPOVER(popover));
        }
    }

    WidgetsData *widgets_data = g_new0(WidgetsData, 1);
    if (!widgets_data) {
        printf("Erro: falha ao alocar WidgetsData\n");
        g_object_unref(provider);
        g_object_unref(css_file);
        return;
    }



    GtkWidget *music_display_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(music_display_content, GTK_ALIGN_BASELINE_FILL);
    gtk_widget_add_css_class(music_display_content, "music_display_content_class");

    GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
    gtk_widget_add_css_class(slider, "slider_main");
    gtk_widget_set_hexpand(slider, TRUE);
    gtk_widget_set_halign(slider, GTK_ALIGN_FILL);
    gtk_range_set_value(GTK_RANGE(slider), get_last_volume());
    gtk_box_append(GTK_BOX(music_display_content), slider);

    GtkWidget *progress_bar = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
    gtk_widget_add_css_class(progress_bar, "progress_bar_class");
    gtk_widget_set_hexpand(progress_bar, TRUE);
    gtk_box_append(GTK_BOX(music_display_content), progress_bar);
    g_signal_connect(progress_bar, "change-value", G_CALLBACK(on_clicked_progess_bar), widgets_data);

    GtkWidget *music_button = gtk_button_new_from_icon_name("media-playback-start");
    gtk_widget_add_css_class(music_button, "music_button_class");
    gtk_widget_set_cursor_from_name(music_button, "hand2");
    gtk_box_append(GTK_BOX(music_display_content), music_button);

    widgets_data->music_display_content = music_display_content;
    widgets_data->volume_slider = slider;
    widgets_data->progress_bar = progress_bar;
    widgets_data->music_button = music_button;
    widgets_data->window_parent = GTK_WIDGET(window);

    AdwClamp *clamp = ADW_CLAMP(adw_clamp_new());
    adw_clamp_set_child(clamp, music_display_content);
    gtk_widget_add_css_class(GTK_WIDGET(clamp), "header_bar");
    gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(clamp));

    GtkWidget *music_holder_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(music_holder_box, TRUE);
    gtk_widget_set_vexpand(music_holder_box, TRUE);
    gtk_widget_add_css_class(music_holder_box, "music_holder_box");
    

    GtkDropTarget *target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
    gtk_drop_target_set_gtypes (target, (GType[1]) { GDK_TYPE_FILE_LIST, }, 1);
    g_signal_connect(target, "drop", G_CALLBACK (on_drop), widgets_data);
    gtk_widget_add_controller (GTK_WIDGET (window), GTK_EVENT_CONTROLLER (target));
    

    GtkWidget *tag_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_add_css_class(tag_box, "tag_box");
    GtkWidget *tag_label_name = gtk_label_new("Music Name");
    gtk_widget_add_css_class(tag_label_name, "tag_label_name");
    gtk_widget_set_hexpand(tag_label_name, TRUE);
    gtk_box_append(GTK_BOX(tag_box), tag_label_name);
    GtkWidget *tag_label_duration = gtk_label_new("Duration");
    gtk_widget_set_margin_end(tag_label_duration, 20);
    gtk_widget_add_css_class(tag_label_duration, "tag_label_duration");
    gtk_box_append(GTK_BOX(tag_box), tag_label_duration);
    gtk_box_append(GTK_BOX(music_holder_box), tag_box);

    GtkWidget *horizontal_separator = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(horizontal_separator, 10);
    gtk_widget_set_margin_end(horizontal_separator, 10);
    gtk_widget_set_hexpand(horizontal_separator, TRUE);
    gtk_widget_set_halign(horizontal_separator, GTK_ALIGN_FILL);
    gtk_widget_set_size_request(horizontal_separator, -1, 1);
    gtk_widget_add_css_class(horizontal_separator, "line_class");

    gtk_box_append(GTK_BOX(music_holder_box), horizontal_separator);

    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_BROWSE);
    gtk_widget_add_css_class(list_box, "list_box");
    widgets_data->list_box = list_box;
    gtk_box_append(GTK_BOX(music_holder_box), list_box);

    AdwPreferencesGroup *prefs_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    gtk_widget_add_css_class(GTK_WIDGET(prefs_group), "prefs_group");
    adw_preferences_group_add(prefs_group, music_holder_box);
    gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(prefs_group));

    g_object_set_data(G_OBJECT(window), "play_selected_music", (gpointer)play_selected_music);
    g_object_set_data(G_OBJECT(window), "widgets_data", widgets_data);
    g_object_set_data(G_OBJECT(window), "music_holder", music_holder_box);

    if (g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_EXISTS)) {
        log_message("folder já existe");
        log_command("Examinando folder...");
        run_subprocess_async("sh src/sh/brokenlinks.sh", NULL, NULL);

    } else {
        g_mkdir_with_parents(SYM_AUDIO_DIR, 0777);
        log_command("Pasta criada com sucesso");
    }
    create_music_list(SYM_AUDIO_DIR, widgets_data, music_holder_box, play_selected_music);

    g_signal_handlers_disconnect_by_func(widgets_data->list_box, G_CALLBACK(play_selected_music), widgets_data);
    g_signal_connect(widgets_data->list_box, "row-activated", G_CALLBACK(play_selected_music), widgets_data);

    g_signal_connect(widgets_data->music_button, "clicked", G_CALLBACK(pause_audio), widgets_data);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), widgets_data);

    g_object_unref(provider);
    g_object_unref(css_file);

    monitor_audio_dir_linkfiles(SYM_AUDIO_DIR, widgets_data);
    monitor_audio_dir(SYM_AUDIO_DIR, widgets_data);

    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
}


int main(int argc, char *argv[]) {
    set_last_volume(get_volume_from_settings());
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
