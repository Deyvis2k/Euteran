#include <gtk/gtk.h>
#include "adwaita.h"
#include "euteran_main_object.h"
#include "src/e_logs.h"
#include "src/ewindows.h"
#include "src/widget_properties.h"
#include "src/e_commandw.h"
#include "src/widgets_devices.h"
#include "src/constants.h"
#include "src/euteran_settings.h"
#include "utils.h"

static void 
on_activate
(
    AdwApplication *app, 
    gpointer user_data
) 
{
    GtkBuilder *builder = gtk_builder_new_from_file("ux/euteran_main.ui");

    EuteranMainObject *widgets_data = euteran_main_object_new();
    euteran_main_object_autoinsert(widgets_data, builder);
    euteran_main_object_set_optional_pointer_object(widgets_data, play_selected_music);

    GtkWidget *window = (GtkWidget *)euteran_main_object_get_widget_at(widgets_data, WINDOW_PARENT);
    GtkWidget *menu_button = BUILDER_GET(builder, "menu_button");

    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));

    GFile *css_file = get_file_from_path();
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_file(provider, css_file);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);


    GtkWidget *button_select_folder = BUILDER_GET(builder, "button_select_folder");
    GtkWidget *button_bindings = BUILDER_GET(builder, "button_bindings");
    GtkWidget *button_devices = BUILDER_GET(builder, "button_devices");

    g_signal_connect(button_select_folder, "clicked", G_CALLBACK(select_folder), widgets_data);
    g_signal_connect(button_devices, "clicked", G_CALLBACK(construct_widget), widgets_data);

    EuteranSettings *current_settings_singleton = euteran_settings_get();

    gtk_window_set_default_size(GTK_WINDOW(window), euteran_settings_get_window_width(current_settings_singleton),
                                euteran_settings_get_window_height(current_settings_singleton));

    GtkWidget *stack_switcher = GTK_WIDGET(euteran_main_object_get_widget_at(widgets_data, STACK_SWITCHER));
    GtkGesture *gesture_right_click_switch = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture_right_click_switch), 3);
    gtk_widget_add_controller(stack_switcher, GTK_EVENT_CONTROLLER(gesture_right_click_switch));
    
    if(!widgets_data || !EUTERAN_IS_MAIN_OBJECT(widgets_data)){
        log_error("Failed to create main object");
    } else {
        g_signal_connect(gesture_right_click_switch, "pressed", G_CALLBACK(on_stack_switcher_right_click), widgets_data);
    }

    gtk_range_set_value(GTK_RANGE(euteran_main_object_get_widget_from_name(widgets_data, builder, "volume_slider")), euteran_settings_get_last_volume(current_settings_singleton));
    GtkWidget *music_holder_box = BUILDER_GET(builder, "music_holder_box");

    GtkDropTarget *target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
    gtk_drop_target_set_gtypes (target, (GType[1]) { GDK_TYPE_FILE_LIST, }, 1);
    gtk_widget_add_controller (GTK_WIDGET (window), GTK_EVENT_CONTROLLER (target));

    g_object_set_data(G_OBJECT(window), "music_holder", music_holder_box);

    if (g_file_test(SYM_AUDIO_DIR, G_FILE_TEST_EXISTS)) {
        log_message("folder já existe");
    } else {
        g_mkdir_with_parents(SYM_AUDIO_DIR, 0777);
        log_command("Pasta criada com sucesso");
    }
    euteran_main_object_load_and_apply_data_json(widgets_data, play_selected_music);
    
    g_signal_connect(target, "drop", G_CALLBACK (on_drop), widgets_data);
    g_signal_connect(euteran_main_object_get_widget_at(widgets_data, PROGRESS_BAR), "change-value", G_CALLBACK(on_clicked_progress_bar), widgets_data);
    g_signal_connect(euteran_main_object_get_widget_at(widgets_data, VOLUME_SLIDER), "value-changed", G_CALLBACK(on_volume_changed), widgets_data);
    g_signal_connect(euteran_main_object_get_widget_at(widgets_data, MUSIC_BUTTON), "clicked", G_CALLBACK(pause_audio), widgets_data);
    g_signal_connect(euteran_main_object_get_widget_at(widgets_data, STOP_BUTTON), "clicked", G_CALLBACK(interrupt_audio), widgets_data);
    g_signal_connect(euteran_main_object_get_widget_at(widgets_data, WINDOW_PARENT), "unrealize", G_CALLBACK(on_window_destroy), widgets_data);
    g_signal_connect(euteran_main_object_get_widget_at(widgets_data, SWITCHER_ADD_BUTTON), "clicked", G_CALLBACK(on_switcher_add_button), widgets_data);

    g_object_unref(provider);
    g_object_unref(css_file);

    monitor_audio_dir_linkfiles(SYM_AUDIO_DIR, widgets_data);
    monitor_audio_dir(SYM_AUDIO_DIR, widgets_data);

    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
}

int main(int argc, char *argv[]) {
    adw_init();
    AdwApplication *app = adw_application_new("com.deyvis2k.Euteran", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
