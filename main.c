#include <gtk/gtk.h>
#include "audio.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtkshortcut.h"
#include "src/e_widgets.h"
#include "src/ewindows.h"
#include "locale.h"
#include "src/widget_properties.h"
#include "adwaita.h"
#include "src/widgets_devices.h"
#include "src/utils.h"

void on_activate(GtkApplication *app, gpointer user_data) {
    AdwApplicationWindow *window = ADW_APPLICATION_WINDOW(adw_application_window_new(app));
    gchar *title;
    #ifdef __linux__
        #include <stdlib.h>
        const char* sessions_names[] = {"i3", "hyprland", "sway", NULL};
        const char *session_name = getenv("DESKTOP_SESSION");
        for (int i = 0; sessions_names[i]; i++) {
            if (strcmp(sessions_names[i], session_name) == 0) {
                title = "Background";
                break;
            }
        }
        if (title == NULL) {
            title = "Euteran";
        }
    #else
        title = "Euteran";
    #endif
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400); 
    gtk_widget_add_css_class(GTK_WIDGET(window), "main_window_class");


    GFile *css_file = get_file_from_path();
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_file(provider, css_file);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(main_box, "main_box");
    adw_application_window_set_content(window, main_box);

    AdwHeaderBar *header_bar = ADW_HEADER_BAR(adw_header_bar_new());
    gtk_widget_add_css_class(GTK_WIDGET(header_bar), "header_bar");
    adw_header_bar_set_show_title(header_bar, FALSE);
    adw_header_bar_set_show_back_button(header_bar, FALSE);
    gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(header_bar));

    GtkWidget *menu_button = gtk_menu_button_new();
    gtk_menu_button_set_label(GTK_MENU_BUTTON(menu_button), "Options");
    adw_header_bar_pack_start(header_bar, menu_button);

    GtkWidget *popover = gtk_popover_new();
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), popover);
    // gtk_widget_add_css_class(popover, "popover");

    GtkWidget *popover_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_popover_set_child(GTK_POPOVER(popover), popover_box);
    // gtk_widget_add_css_class(popover_box, "popover_box");

    const char* popover_names[] = {"Select Folder", "Bindings", "Devices", NULL};
    for (int i = 0; popover_names[i]; i++) {
        GtkWidget *button = gtk_button_new_with_label(popover_names[i]);
        gtk_widget_set_hexpand(button, FALSE);
        // gtk_widget_add_css_class(button, "popover_button");
        gtk_box_append(GTK_BOX(popover_box), button);

        if (strcmp(popover_names[i], "Select Folder") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(select_folder), window);
            gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
        } else if (strcmp(popover_names[i], "Devices") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(construct_widget), window);
            gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
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
    
    GtkDropTarget *target =
      gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);

      gtk_drop_target_set_gtypes (target, (GType [2]) {
        G_TYPE_STRING,
        G_TYPE_FILE,
      }, 2);

    g_signal_connect(target, "drop", G_CALLBACK(on_drop), widgets_data);
    gtk_widget_add_controller(GTK_WIDGET(music_holder_box), GTK_EVENT_CONTROLLER(target));

   
    GtkWidget *tag_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_add_css_class(tag_box, "tag_box");
    GtkWidget *tag_label_name = gtk_label_new("Music Name");
    gtk_widget_add_css_class(tag_label_name, "tag_label_name");
    gtk_widget_set_hexpand(tag_label_name, TRUE);
    gtk_box_append(GTK_BOX(tag_box), tag_label_name);
    GtkWidget *tag_label_duration = gtk_label_new("Duration");
    gtk_widget_set_margin_end(tag_label_duration, 7);
    gtk_widget_add_css_class(tag_label_duration, "tag_label_duration");
    gtk_box_append(GTK_BOX(tag_box), tag_label_duration);
    gtk_box_append(GTK_BOX(music_holder_box), tag_box);

    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_BROWSE);
    gtk_widget_add_css_class(list_box, "list_box");
    widgets_data->list_box = list_box;
    gtk_box_append(GTK_BOX(music_holder_box), list_box);

    

    AdwPreferencesGroup *prefs_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    gtk_widget_add_css_class(GTK_WIDGET(prefs_group), "prefs_group");
    adw_preferences_group_add(prefs_group, music_holder_box);
    gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(prefs_group));

    // AdwBreakpoint *breakpoint = adw_breakpoint_new(
    //     adw_breakpoint_condition_parse("max-width: 600px")
    // );
    // adw_breakpoint_add_setter(breakpoint, G_OBJECT(clamp), "maximum-size", (GValue *)g_variant_new_int32(400));
    // adw_application_window_add_breakpoint(window, breakpoint);

    // Store data
    g_object_set_data(G_OBJECT(window), "play_selected_music", (gpointer)play_selected_music);
    g_object_set_data(G_OBJECT(window), "widgets_data", widgets_data);
    g_object_set_data(G_OBJECT(window), "music_holder", music_holder_box);

    if (g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_EXISTS)) {
        g_print(CYAN_COLOR "[INFO] Pasta ja existe\n" RESET_COLOR);
        g_print(GREEN_COLOR "[COMMAND] Examinando folder..\n" RESET_COLOR);
        system("sh src/sh/brokenlinks.sh");
    } else {
        g_mkdir_with_parents(SYM_AUDIO_DIR, 0777);
        g_print(GREEN_COLOR "[COMMAND] Pasta criada\n" RESET_COLOR);
    }
    create_music_list(SYM_AUDIO_DIR, widgets_data, music_holder_box, play_selected_music);

    g_signal_handlers_disconnect_by_func(widgets_data->list_box, G_CALLBACK(play_selected_music), widgets_data);
    g_signal_connect(widgets_data->list_box, "row-activated", G_CALLBACK(play_selected_music), widgets_data);

    g_signal_connect(widgets_data->music_button, "clicked", G_CALLBACK(pause_audio), widgets_data);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), widgets_data);

    g_object_unref(provider);
    g_object_unref(css_file);

    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
}


int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "en_US.UTF-8");
    // setenv("GTK_THEME", "Catppuccin-Mocha", 1);
    set_last_volume(get_volume_from_settings());
    GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
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
