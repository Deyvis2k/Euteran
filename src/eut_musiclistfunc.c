#include "eut_musiclistfunc.h"
#include "eut_constants.h"
#include "eut_logs.h"
#include "eut_main_object.h"
#include "gdk/gdk.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "adwaita.h"
#include "eut_utils.h"
#include "eut_audiolinux.h"

#define _(s) (s)

static gboolean is_pressed = FALSE;

void clear_container_children(GtkWidget *container) {
    GtkWidget *child = gtk_widget_get_first_child(container);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);

        if(child != NULL)
            gtk_widget_unparent(child); 
        child = next;
    }
}

static void 
on_remove_music(
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
dialog_cb (AdwAlertDialog *dialog,
           GAsyncResult   *result,
           GtkWidget       *self)
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
    dialog = adw_alert_dialog_new (_("Remove Music?"), NULL);

    adw_alert_dialog_format_body (ADW_ALERT_DIALOG (dialog),
                                _("Are you sure you want to remove\n %s?"),
                                music_name);

    adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog),
                                  "cancel",  _("_Cancel"),
                                  "remove", _("_Remove"),
                                  NULL);

    adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog),
                                            "remove",
                                            ADW_RESPONSE_DESTRUCTIVE);


    g_object_set_data (G_OBJECT (dialog), "container", container);

    adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "cancel");
    adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");

    adw_alert_dialog_choose (ADW_ALERT_DIALOG (dialog), GTK_WIDGET (container->window_parent),
                           NULL, (GAsyncReadyCallback) dialog_cb, NULL);

}


static void
on_pressed_right_click_event(
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
on_removed_music_event_window(
    GtkWidget   *widget,
    gpointer    user_data
)
{
    EMusicContainer *container = (EMusicContainer *)user_data;
    g_assert(GTK_IS_WIDGET(container->window_parent) && GTK_IS_WINDOW(container->window_parent));
    
    gtk_widget_queue_allocate(container->window_parent);
    gtk_widget_queue_resize(container->window_parent);
}

static void
free_music_container(
    EMusicContainer *container
)
{
    if(!container) return;

    log_info("Freeing music container: %s", container->path ? container->path : "NULL");

    if(container->path) g_free(container->path);
    g_free(container);
}

void create_music_list(
    const gchar              *path, 
    EuteranMainObject        *widgets_data, 
    PlayMusicFunc            play_selected_music
) 
{
    
    if (!widgets_data) {
        log_error("Widgets_data ou seus componentes são inválidos");
        return;
    }
    
    GtkWidget *stack = GTK_WIDGET(euteran_main_object_get_widget_at(widgets_data, STACK));

    if(!stack){
        log_error("Stack nao encontrada");
        return;
    }

    GtkWidget *scrolled_window = gtk_stack_get_visible_child(GTK_STACK(stack));

    if(!scrolled_window || !GTK_IS_SCROLLED_WINDOW(scrolled_window)){
        log_error("Scrolled window nao encontrada");
        return;
    }

    GtkWidget *list_box = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled_window));

    if(GTK_IS_VIEWPORT(list_box)){
        list_box = gtk_viewport_get_child(GTK_VIEWPORT(list_box));
    }
    

    GList *new_music_list = list_files_musics(path);
    if (new_music_list == NULL) {
        log_warning("Nenhuma musica encontrada");
        return;
    }

    gtk_list_box_remove_all(GTK_LIST_BOX(list_box));

    for (GList *node = new_music_list; node != NULL; node = node->next) {
        music_t *temp_music = (music_t *)node->data;

        if(!temp_music->name){
            log_error("Musica sem nome");
            continue;
        }

        GtkWidget *row = gtk_list_box_row_new();
        if (!row) {
            log_error("Erro ao criar row");
            continue;
        }


        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_set_hexpand(row_box, TRUE);
        gtk_widget_set_margin_top(row_box, 0);
        gtk_widget_set_margin_bottom(row_box, 0);
        gtk_widget_add_css_class(row_box, "row_box_class");


        GtkGesture *event_mouse = gtk_gesture_click_new();
        gtk_gesture_single_set_button (GTK_GESTURE_SINGLE(event_mouse), 3);
        
        EMusicContainer *music_container = g_new0(EMusicContainer, 1);

        music_container->window_parent = GTK_WIDGET(euteran_main_object_get_widget_at(widgets_data, WINDOW_PARENT));
        music_container->row = row;
        music_container->row_box = row_box;
        music_container->list_box = list_box;
        
        gtk_widget_add_controller(music_container->row_box, GTK_EVENT_CONTROLLER(event_mouse));
        g_signal_connect(event_mouse, "pressed", G_CALLBACK(on_pressed_right_click_event), music_container);
        g_signal_connect(music_container->row_box, "destroy", G_CALLBACK(on_removed_music_event_window), music_container);
        

        GtkWidget *label = gtk_label_new(temp_music->name);
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        gtk_label_set_xalign(GTK_LABEL(label), 1.0);
        gtk_widget_set_halign(label, GTK_ALIGN_BASELINE_CENTER);
        gtk_widget_set_hexpand(label, TRUE); 
        gtk_box_append(GTK_BOX(row_box), label);
        
        music_container->path = g_strconcat(SYM_AUDIO_DIR, temp_music->name, NULL);

        GtkWidget *line = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_size_request(line, 1, -1);
        gtk_widget_set_halign(line, GTK_ALIGN_END);
        gtk_widget_add_css_class(line, "line_class");
        gtk_box_append(GTK_BOX(row_box), line);

        const char *duration_str = cast_double_to_string(temp_music->duration);
        GtkWidget *duration = gtk_label_new(duration_str ? duration_str : "0.0");        
        gtk_widget_set_size_request(duration, 75, -1);
        gtk_label_set_ellipsize(GTK_LABEL(duration), PANGO_ELLIPSIZE_END);
        gtk_label_set_xalign(GTK_LABEL(duration), 0.0);
        gtk_box_append(GTK_BOX(row_box), duration);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
        gtk_list_box_insert(GTK_LIST_BOX(list_box), row, -1);

        g_object_set_data(G_OBJECT(row), "music_name", temp_music->name);
        g_object_set_data(G_OBJECT(row), "music_duration", g_strdup_printf("%.2f", temp_music->duration));

        g_object_set_data_full(
            G_OBJECT(row),
            "music_container",
            music_container,
            (GDestroyNotify)free_music_container
        );
    }

    g_list_free(new_music_list);

    g_signal_handlers_disconnect_by_func(list_box, G_CALLBACK(play_selected_music), widgets_data);
    g_signal_connect(list_box, "row-activated", G_CALLBACK(play_selected_music), widgets_data);
}

gboolean 
add_music_to_list(
    gpointer user_data,
    const gchar *path,
    PlayMusicFunc play_selected_music
){
    gchar *clean_uri = NULL;
    GError *error = NULL;

    if (g_path_is_absolute(path)) {
        clean_uri = g_strdup(path);
    } else {
        clean_uri = g_filename_from_uri(path, NULL, &error);
        if (clean_uri == NULL) {
            log_error("Erro ao decodificar URI: %s", error->message);
            g_error_free(error);
            return FALSE;
        }
    }
    g_strchomp(clean_uri);

    GFile *gfile = g_file_new_for_path(clean_uri);
    if (!g_file_query_exists(gfile, NULL)) {
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

    if (ext == NULL || *(ext + 1) == '\0') {
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

    gchar *link_path = g_strdup_printf("%s%s", SYM_AUDIO_DIR, FILENAME);
    if (g_file_test(link_path, G_FILE_TEST_EXISTS)) {
        log_error("Arquivo %s já existe", FILENAME);
        g_free(link_path);
        g_free(clean_uri);
        return FALSE;
    }

    GFile *source_file = g_file_new_for_path(clean_uri);
    GFile *link_file = g_file_new_for_path(link_path);
    if (!g_file_make_symbolic_link(link_file, clean_uri, NULL, &error)) {
        log_error("Erro ao criar symlink: %s", error->message);
        g_error_free(error);
        g_object_unref(source_file);
        g_object_unref(link_file);
        g_free(clean_uri);
        g_free(link_path);
        return FALSE;
    }
    g_object_unref(source_file);
    g_object_unref(link_file);


    EuteranMainObject *widgets_data = user_data;
    GtkWidget *stack = (GtkWidget *)euteran_main_object_get_widget_at(widgets_data, STACK);
    if (!stack) {
        log_error("Erro ao obter stack");
        return FALSE;
    }
    
    GtkWidget *scrolled_window = gtk_stack_get_visible_child(GTK_STACK(stack));
    if (!scrolled_window || !GTK_IS_SCROLLED_WINDOW(scrolled_window)) {
        log_error("Erro ao obter scrolled window");
        return FALSE;
    }

    GtkWidget *list_box = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled_window));
    if (!list_box) {
        log_error("Erro ao obter list box");
        return FALSE;
    }

    if(GTK_IS_VIEWPORT(list_box)){
        list_box = gtk_viewport_get_child(GTK_VIEWPORT(list_box));
    }

    if(!GTK_IS_LIST_BOX(list_box)){
        log_error("Erro ao obter list box");
        return FALSE;
    }


    GtkWidget *row = gtk_list_box_row_new();
    if (!row) {
        log_error("Erro ao criar row");
        return FALSE;
    }

    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_hexpand(row_box, TRUE);
    gtk_widget_set_margin_top(row_box, 0);
    gtk_widget_set_margin_bottom(row_box, 0);
    gtk_widget_add_css_class(row_box, "row_box_class");


    GtkGesture *event_mouse = gtk_gesture_click_new();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE(event_mouse), 3);
    
    EMusicContainer *music_container = g_new0(EMusicContainer, 1);

    music_container->window_parent = (GtkWidget *)euteran_main_object_get_widget_at(widgets_data, WINDOW_PARENT);
    music_container->row = row;
    music_container->row_box = row_box;
    music_container->list_box = list_box;
    music_container->path = g_strconcat(SYM_AUDIO_DIR, FILENAME, NULL);
    
    gtk_widget_add_controller(music_container->row_box, GTK_EVENT_CONTROLLER(event_mouse));
    g_signal_connect(event_mouse, "pressed", G_CALLBACK(on_pressed_right_click_event), music_container);
    g_signal_connect(music_container->row_box, "destroy", G_CALLBACK(on_removed_music_event_window), music_container);


    GtkWidget *label = gtk_label_new(FILENAME);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(label), 1.0);
    gtk_widget_set_halign(label, GTK_ALIGN_BASELINE_CENTER);
    gtk_widget_set_hexpand(label, TRUE); 
    gtk_box_append(GTK_BOX(row_box), label);

    GtkWidget *line = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(line, 1, -1);
    gtk_widget_set_halign(line, GTK_ALIGN_END);
    gtk_widget_add_css_class(line, "line_class");
    gtk_box_append(GTK_BOX(row_box), line);
    
    double duration_ = get_duration(link_path);
    const char *duration_str = cast_double_to_string(duration_);
    GtkWidget *duration = gtk_label_new(duration_str ? duration_str : "0.0");
    gtk_widget_set_size_request(duration, 75, -1);
    gtk_label_set_ellipsize(GTK_LABEL(duration), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(duration), 0.0);
    gtk_box_append(GTK_BOX(row_box), duration);

    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
    gtk_list_box_insert(GTK_LIST_BOX(list_box), row, -1);

    g_object_set_data(G_OBJECT(row), "music_name", g_strdup(path));
    g_object_set_data(G_OBJECT(row), "music_duration", g_strdup_printf("%.2f", duration_));
    g_object_set_data_full(
        G_OBJECT(row),
        "music_container",
        music_container,
        (GDestroyNotify)free_music_container
    );

    g_free(link_path);
    g_free(clean_uri);

    
    g_signal_handlers_disconnect_by_func(list_box, G_CALLBACK(play_selected_music), widgets_data);
    g_signal_connect(list_box, "row-activated", G_CALLBACK(play_selected_music), widgets_data);


    return TRUE;
}
