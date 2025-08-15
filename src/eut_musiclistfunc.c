#include "eut_musiclistfunc.h"
#include "eut_constants.h"
#include "eut_logs.h"
#include "eut_main_object.h"
#include "gdk/gdk.h"
#include "gio/gio.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "adwaita.h"
#include "eut_utils.h"
#include "eut_audiolinux.h"
#include "eut_widgetsfunctions.h"

#define _(s) (s)

static gboolean is_pressed = FALSE;

static void 
on_remove_music
(
    gpointer    user_data
) 
{
    EMusicContainer *container = (EMusicContainer *)user_data;
    printf("Removendo musica: %s\n", container->path);
    if(unlink(container->path) != 0){
        log_error("Erro ao remover musica: %s", g_strerror(errno));
        return;
    }

    gtk_list_box_remove(GTK_LIST_BOX(container->list_box), container->row);
}


static void
dialog_cb 
(
    AdwAlertDialog *dialog,
    GAsyncResult   *result,
    GtkWidget      *self
)
{
    is_pressed = FALSE;
    EMusicContainer *container = (EMusicContainer *) 
        g_object_get_data (G_OBJECT (dialog), "container");

    const char *response = adw_alert_dialog_choose_finish (dialog, result);
    if (g_strcmp0 (response, "remove") == 0) {
        on_remove_music(container);
    }
}



static void 
create_remove_music_button(
    const char          *music_name,
    EMusicContainer     *container,
    gpointer            user_data
){
    if(is_pressed) {
        return;
    }
    is_pressed = TRUE;
    AdwDialog *dialog;
    dialog = adw_alert_dialog_new (_("Remove Music?"), nullptr);

    adw_alert_dialog_format_body (ADW_ALERT_DIALOG (dialog),
                                _("Are you sure you want to remove\n %s?"),
                                music_name);

    adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog),
                                  "cancel",  _("_Cancel"),
                                  "remove", _("_Remove"),
                                  nullptr);

    adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog),
                                            "remove",
                                            ADW_RESPONSE_DESTRUCTIVE);


    g_object_set_data (G_OBJECT (dialog), "container", container);

    adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "cancel");
    adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");

    adw_alert_dialog_choose (ADW_ALERT_DIALOG (dialog), GTK_WIDGET (container->window_parent),
                           nullptr, (GAsyncReadyCallback) dialog_cb, nullptr);

}


static void
on_pressed_right_click_event
(
    GtkGestureClick     *gesture,
    int                 n_press,
    double              x,
    double              y,
    gpointer            user_data
){
    if (gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture)) == GDK_BUTTON_SECONDARY) {
        EMusicContainer *container = (EMusicContainer *)user_data;

        GtkWidget *label_music_name = gtk_widget_get_first_child(container->row_box);

        if (!GTK_IS_LABEL(label_music_name)) {
            log_error("O primeiro widget do GtkBox nao eh um GtkLabel");
            return;
        }

        const char *music_name = gtk_label_get_text(GTK_LABEL(label_music_name));
        
        create_remove_music_button(music_name, container, user_data);
    }
}

static void
on_removed_music_event_window
(
    GtkWidget   *widget,
    gpointer    user_data
)
{
    g_return_if_fail(user_data != nullptr);
    EMusicContainer *container = (EMusicContainer *)user_data;

    g_return_if_fail(container->window_parent != nullptr);
    
    gtk_widget_queue_allocate(container->window_parent);
    gtk_widget_queue_resize(container->window_parent);
}

void
free_music_container(
    EMusicContainer *container
)
{
    if(!container) return;
    if(container->path) g_free(container->path);
    g_free(container);
}

void 
create_music_row
(
    EuteranMainObject *main_object,
    GtkListBox        *list_box,
    const gchar       *music_filename,
    const gchar       *music_duration_raw,
    gdouble           music_duration
)
{
       
        GtkWidget *row = gtk_list_box_row_new();

        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_set_hexpand(row_box, TRUE);
        gtk_widget_set_margin_top(row_box, 0);
        gtk_widget_set_margin_bottom(row_box, 0);
        gtk_widget_add_css_class(row_box, "row_box_class");


        GtkGesture *event_mouse = gtk_gesture_click_new();
        gtk_gesture_single_set_button (GTK_GESTURE_SINGLE(event_mouse), 3);
        
        EMusicContainer *music_container = g_new0(EMusicContainer, 1);

        music_container->window_parent = GTK_WIDGET(main_object);
        music_container->row = row;
        music_container->row_box = row_box;
        music_container->list_box = (GtkWidget*)list_box;
        
        gtk_widget_add_controller(music_container->row_box, GTK_EVENT_CONTROLLER(event_mouse));
        g_signal_connect(event_mouse, "pressed", G_CALLBACK(on_pressed_right_click_event), music_container);
        g_signal_connect(music_container->row_box, "destroy", G_CALLBACK(on_removed_music_event_window), music_container);
        

        GtkWidget *label = gtk_label_new(music_filename);
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
        gtk_widget_set_halign(label, GTK_ALIGN_BASELINE_CENTER);
        gtk_widget_set_hexpand(label, TRUE); 
        gtk_box_append(GTK_BOX(row_box), label);
        
        music_container->path = g_strconcat(SYM_AUDIO_DIR, music_filename, nullptr);

        GtkWidget *line = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_size_request(line, 1, -1);
        gtk_widget_set_halign(line, GTK_ALIGN_END);
        gtk_widget_add_css_class(line, "line_class");
        gtk_box_append(GTK_BOX(row_box), line);

        const char *duration_str = cast_double_to_string(music_duration);
        GtkWidget *duration = gtk_label_new(duration_str ? duration_str : "0.0");        
        gtk_widget_set_size_request(duration, 75, -1);
        gtk_label_set_ellipsize(GTK_LABEL(duration), PANGO_ELLIPSIZE_END);
        gtk_label_set_xalign(GTK_LABEL(duration), 0.0f);
        gtk_box_append(GTK_BOX(row_box), duration);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
        gtk_list_box_insert(list_box, row, -1);

        g_object_set_data(G_OBJECT(row), "music_name", g_strdup(music_filename));
        g_object_set_data(G_OBJECT(row), "music_duration", g_strdup(music_duration_raw));

        g_object_set_data_full(
            G_OBJECT(row),
            "music_container",
            music_container,
            (GDestroyNotify)free_music_container
        );
}

void create_music_list
(
    const gchar              *path, 
    EuteranMainObject        *main_object 
) 
{
    if (!main_object || !EUTERAN_IS_MAIN_OBJECT(main_object)) {
        log_error("Widgets_data ou seus componentes são inválidos");
        return;
    }
    GtkListBox *list_box = euteran_main_object_get_focused_list_box(main_object);

    GList *new_music_list = list_files_musics(path);
    if (new_music_list == nullptr) {
        log_warning("Nenhuma musica encontrada");
        return;
    }

    for (GList *node = new_music_list; node != nullptr; node = node->next) {
        music_t *temp_music = (music_t *)node->data;

        if(!temp_music->name){
            log_error("Musica sem nome");
            continue;
        }

        if(euteran_main_object_list_box_contains_music(main_object, temp_music->name)){
            log_info("Music já adicionada %s", temp_music->name);
            continue;
        }
        gchar *music_duration_raw = g_strdup_printf("%.2f", temp_music->duration);
        create_music_row(main_object, list_box, temp_music->name, music_duration_raw, temp_music->duration);

    }

    g_list_free(new_music_list);

    g_signal_handlers_disconnect_by_func(list_box, G_CALLBACK(play_selected_music), main_object);
    g_signal_connect(list_box, "row-activated", G_CALLBACK(play_selected_music), main_object);
}

gboolean 
add_music_to_list
(
    gpointer user_data,
    const gchar *path
){
    if(!user_data || !EUTERAN_IS_MAIN_OBJECT(user_data)){
        log_error("Widgets_data ou seus componentes são inválidos");
        return FALSE;
    }
    EuteranMainObject *widgets_data = user_data;
    

    gchar *clean_uri = nullptr;
    GError *error = nullptr;

    if (g_path_is_absolute(path)) {
        clean_uri = g_strdup(path);
    } else {
        clean_uri = g_filename_from_uri(path, nullptr, &error);
        if (clean_uri == nullptr) {
            log_error("Erro ao decodificar URI: %s", error->message);
            g_error_free(error);
            return FALSE;
        }
    }
    g_strchomp(clean_uri);

    GFile *gfile = g_file_new_for_path(clean_uri);
    if (!g_file_query_exists(gfile, nullptr)) {
        log_error("Arquivo %s nao existe", clean_uri);
        g_object_unref(gfile);
        g_free(clean_uri);
        return FALSE;
    }
    g_object_unref(gfile);

    FILE *test_file = fopen(clean_uri, "r");
    if (!test_file) {
        log_error("Erro ao abrir arquivo %s: %s", clean_uri, strerror(errno));
        g_free(clean_uri);
        return FALSE;
    }
    fclose(test_file);



    const char *ext = strrchr(clean_uri, '.') - 1;

    if (ext == nullptr || *(ext + 1) == '\0') {
        log_error("Extensão inválida");
        g_free(clean_uri);
        return FALSE;
    }

    ext++; 

    if (!IS_ALLOWED_EXTENSION(ext)) {
        log_error("Extensão inválida: %s", ext);
        g_free(clean_uri);
        return FALSE;
    }

    const char *FILENAME = strrchr(clean_uri, '/') + 1;
    
    if(euteran_main_object_list_box_contains_music(widgets_data, FILENAME)){
        log_error("Musica %s ja esta na lista", FILENAME);
        g_free(clean_uri);
        return FALSE;
    }
    gchar *link_path = g_strdup_printf("%s%s", SYM_AUDIO_DIR, FILENAME);
    GFile *link_file = g_file_new_for_path(link_path);
    if(!g_file_test(link_path, G_FILE_TEST_EXISTS)){
        if (!g_file_make_symbolic_link(link_file,clean_uri, nullptr, &error)) {
            log_error("Erro ao criar pasta: %s", error->message);
            g_error_free(error);
            g_object_unref(link_file);
            g_free(clean_uri);
            g_free(clean_uri);
            return FALSE;
        }
    }
    g_object_unref(link_file);


    GtkListBox *list_box = euteran_main_object_get_focused_list_box(widgets_data);
    double duration = get_duration(clean_uri);
    const gchar *duration_raw = g_strdup_printf("%.2f", duration);

    create_music_row(widgets_data, list_box, FILENAME, duration_raw, duration);
    

    g_free(clean_uri);
    g_free(link_path);
    
    g_signal_handlers_disconnect_by_func(list_box, G_CALLBACK(play_selected_music), widgets_data);
    g_signal_connect(list_box, "row-activated", G_CALLBACK(play_selected_music), widgets_data);


    return TRUE;
}
