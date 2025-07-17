#include "euteran_main_object.h"
#include "constants.h"
#include "e_logs.h"
#include "glib.h"
#include "glib/gstdio.h"
#include "gtk/gtk.h"
#include "json-glib/json-glib.h"
#include "utils.h"
#include "euteran_dialogs.h"

struct _EuteranMainObject {
    GObject parent_instance;

    GList *euteran_widgets_list;
    double music_duration;
    GTimer *timer;
    double offset_time;
    void *optional_object;
};

G_DEFINE_TYPE(EuteranMainObject, euteran_main_object, G_TYPE_OBJECT);

static void euteran_main_object_init(EuteranMainObject *self) {
    self->euteran_widgets_list = NULL;
    self->music_duration = 0.0;
    self->timer = g_timer_new();
    self->offset_time = 0.0;
    self->optional_object = NULL;
}

static void euteran_main_object_dispose(GObject *object) {
    EuteranMainObject *self = EUTERAN_MAIN_OBJECT(object);

    if (self->timer) {
        g_timer_destroy(self->timer);
        self->timer = NULL;
    }

    if (self->euteran_widgets_list) {
        g_list_free_full(self->euteran_widgets_list, g_object_unref);
        self->euteran_widgets_list = NULL;
    }

    G_OBJECT_CLASS(euteran_main_object_parent_class)->dispose(object);
}

void euteran_main_object_clear(EuteranMainObject *self) {
     g_return_if_fail(EUTERAN_IS_MAIN_OBJECT(self));

    if (self->euteran_widgets_list != NULL) {
        g_list_free(self->euteran_widgets_list); 
        self->euteran_widgets_list = NULL;
    }

    if (self->timer != NULL) {
        g_timer_destroy(self->timer);
        self->timer = NULL;
    }
    self->optional_object = NULL;
    self->music_duration = 0.0;
    self->offset_time = 0.0;
}

gboolean
euteran_main_object_is_valid(EuteranMainObject *self) {
    if(
        EUTERAN_IS_MAIN_OBJECT(self) &&
        self->euteran_widgets_list != NULL &&
        self->timer != NULL
    ) 
    {
        return TRUE;    
    }

    return FALSE;
}


static void euteran_main_object_class_init(EuteranMainObjectClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = euteran_main_object_dispose;
}

void euteran_main_object_add_widget(
    EuteranMainObject *self, 
    GtkWidget *widget, 
    const gchar *name
) 
{
    g_return_if_fail(EUTERAN_IS_MAIN_OBJECT(self));
    g_return_if_fail(GTK_IS_WIDGET(widget));

    gtk_widget_set_name(widget, name);
    g_object_ref(widget);
    self->euteran_widgets_list = g_list_append(self->euteran_widgets_list, widget);
}


void euteran_main_object_remove_widget(EuteranMainObject *self, GtkWidget *widget) {
    g_return_if_fail(EUTERAN_IS_MAIN_OBJECT(self));
    g_return_if_fail(GTK_IS_WIDGET(widget));

    self->euteran_widgets_list = g_list_remove(self->euteran_widgets_list, widget);
}

GObject *
euteran_main_object_get_widget_from_name(
    EuteranMainObject    *self, 
    GtkBuilder           *builder, 
    const gchar          *name
) 
{
    GObject *widget = NULL;
    widget = gtk_builder_get_object(builder, name);
    if(widget == NULL) {
        log_error("Widget %s not found", name);
        return NULL;
    } 
    return widget;
}

GObject *
euteran_main_object_get_widget_at(
    EuteranMainObject *self, 
    WIDGETS           id
) 
{
    if(!EUTERAN_IS_MAIN_OBJECT(self)) {
        log_error("EuteranMainObject não é uma instância válida.");
        return NULL;
    }
    GObject *widget = NULL;
    if(self->euteran_widgets_list != NULL) {
        widget = g_list_nth_data(self->euteran_widgets_list, id);
    }
    return widget;
}


void 
euteran_main_object_autoinsert(
    EuteranMainObject *self, 
    GtkBuilder        *builder 
)
{
    for(int i = 0; i < MAX_WIDGETS; i++) {
        gpointer widget = euteran_main_object_get_widget_from_name(self, builder, widgets_names[i]);
        if(widget == NULL) {
            log_error("Widget %s not found", widgets_names[i]);
            continue;
        }
        euteran_main_object_add_widget(self, GTK_WIDGET(widget), widgets_names[i]);
    }   
}

EuteranMainObject *euteran_main_object_new(void) {
    return g_object_new(EUTERAN_TYPE_MAIN_OBJECT, NULL);
}


void euteran_main_object_set_duration(EuteranMainObject *self, double duration) {
    self->music_duration = duration;
}

void euteran_main_object_set_timer(EuteranMainObject *self, GTimer *timer) {
    g_return_if_fail(EUTERAN_IS_MAIN_OBJECT(self));

    if (self->timer != NULL) {
        g_timer_destroy(self->timer);
    }

    self->timer = timer;
}

void euteran_main_object_set_offset_time(EuteranMainObject *self, double offset_time) {
    self->offset_time = offset_time;
}

double euteran_main_object_get_offset_time(EuteranMainObject *self) {
    return self->offset_time;
}

double euteran_main_object_get_duration(EuteranMainObject *self) {
    return self->music_duration;
}

GTimer *euteran_main_object_get_timer(EuteranMainObject *self) {
    return self->timer;
}

GList *euteran_main_object_get_euteran_widgets_list(EuteranMainObject *self) {
    return self->euteran_widgets_list;
}

void euteran_main_object_set_optional_pointer_object(EuteranMainObject *self, void *optional_pointer_object) {
    self->optional_object = optional_pointer_object;
}

void *euteran_main_object_get_optional_pointer_object(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self) && self->optional_object != NULL, NULL);

    return self->optional_object;
}

void
euteran_main_object_save_data_json (EuteranMainObject *self)
{
    g_return_if_fail (EUTERAN_IS_MAIN_OBJECT (self));

    GtkWidget        *stack  = (GtkWidget *)euteran_main_object_get_widget_at (self, STACK);
    GtkSelectionModel*pages  = gtk_stack_get_pages (GTK_STACK (stack));
    guint             n_items = g_list_model_get_n_items (G_LIST_MODEL (pages));

    if(n_items == 0) {
        log_info("No music data to save");
        return;
    }

    log_warning("Saving this much music data: %d", n_items);

    JsonBuilder *b = json_builder_new ();

    json_builder_begin_object (b);
    json_builder_set_member_name (b, "music_data");
    json_builder_begin_array (b);

    for (guint i = 0; i < n_items; i++) {

        GtkStackPage *page = g_list_model_get_item (G_LIST_MODEL (pages), i);
        if (!page)
            continue;


        const gchar *list_box_name = gtk_stack_page_get_title(page);
        if(!list_box_name){
            log_error("Failed to get list box name");
            continue;
        }

        log_info("Saving music data for list box: %s", list_box_name);

        json_builder_begin_object (b);
        json_builder_set_member_name (b, "list_box_name");
        json_builder_add_string_value (b, list_box_name);

        json_builder_set_member_name (b, "music_list_path");
        json_builder_begin_array (b);

        GtkWidget *scrolled_window = gtk_stack_page_get_child (page);
        if (GTK_IS_SCROLLED_WINDOW (scrolled_window)) {

            GtkWidget *list_box = gtk_scrolled_window_get_child (GTK_SCROLLED_WINDOW (scrolled_window));
            if (GTK_IS_VIEWPORT (list_box))
                list_box = gtk_viewport_get_child (GTK_VIEWPORT (list_box));

            if (GTK_IS_LIST_BOX (list_box)) {
                for (GtkWidget *row = gtk_widget_get_first_child (list_box);
                     row != NULL;
                     row = gtk_widget_get_next_sibling (row))
                {
                    GtkWidget *row_box = gtk_widget_get_first_child (row);
                    if (!GTK_IS_BOX (row_box))
                        continue;
                    
                    GtkWidget *label = gtk_widget_get_first_child (row_box);
                    GtkWidget *music_duration = gtk_widget_get_last_child (row_box);

                    const gchar *music_path = gtk_label_get_text (GTK_LABEL (label));
                    const gchar *music_duration_text = gtk_label_get_text (GTK_LABEL (music_duration));

                    if(!music_path || !music_duration_text){
                        log_error("Failed to get music path or duration");
                        continue;
                    }

                    json_builder_begin_object (b);
                    json_builder_set_member_name (b, "music_path");
                    json_builder_add_string_value (b, music_path);
                    json_builder_set_member_name (b, "music_duration");
                    json_builder_add_string_value (b, music_duration_text);
                    json_builder_end_object (b);
                }
            }
        }
        
        json_builder_end_array (b);
        json_builder_end_object (b);
    }

    json_builder_end_array (b);   
    json_builder_end_object (b);  

    JsonNode     *root = json_builder_get_root (b);   
    JsonGenerator*gen  = json_generator_new ();
    json_generator_set_root (gen, root);

    gchar *path = g_build_filename (CONFIGURATION_DIR, "music_data.json", NULL);
    g_mkdir_with_parents (g_path_get_dirname (path), 0700);

    GError *err = NULL;
    if (!json_generator_to_file (gen, path, &err))
        g_warning ("Falha ao gravar %s: %s", path, err->message);
    else
        g_print ("JSON salvo em %s\n", path);

    g_free (path);
    g_object_unref (gen);
    json_node_free (root);
    g_object_unref (b);
}

GtkWidget*
euteran_main_object_get_list_box
(
    GtkSelectionModel *pages,
    guint             index
)
{
    GtkWidget *list_box = NULL;

    GtkStackPage *page = g_list_model_get_item (G_LIST_MODEL (pages), index);
    if(!page || !GTK_IS_STACK_PAGE(page)){
        return NULL;
    }

    GtkWidget *scrolled_window = gtk_stack_page_get_child (page);
    if(!scrolled_window || !GTK_IS_SCROLLED_WINDOW(scrolled_window)){
        return NULL;
    }

    list_box = gtk_scrolled_window_get_child (GTK_SCROLLED_WINDOW (scrolled_window));
    if(!list_box){
        return NULL;
    }

    if(GTK_IS_VIEWPORT(list_box)){
        list_box = gtk_viewport_get_child (GTK_VIEWPORT (list_box));
    }

    if(!GTK_IS_LIST_BOX(list_box)){
        return NULL;
    }


    return list_box;
}

static void
free_music_container(
    EuteranDialogContainer *container
)
{
    if(!container) return;

    log_info("Freeing music container: %s", container->path ? container->path : "NULL");

    if(container->path) g_free(container->path);
    g_free(container);
}

static void
free_music_list_data(MusicListData *data)
{
    if (!data) return;
    
    g_free(data->name);
    
    g_list_free(data->music_list);

    g_free(data);
}

static MusicDataSave * 
music_list_data_any(const char *name_to_analyze, MusicListData *data)
{
    for (GList *l = data->music_list; l != NULL; l = l->next) {
        MusicDataSave *music_data = l->data;
        if (g_strcmp0(name_to_analyze, music_data->music_path) == 0) {
            return music_data;
        }
    }
    return NULL;
}

static GList*
load_music_data_from_json(const gchar *json_path, GError **error, GList *symlinks_musics_to_analyze)
{
    g_autoptr(JsonParser) parser = json_parser_new();
    
    if (!json_parser_load_from_file(parser, json_path, error)) {
        return NULL;
    }
    
    g_autoptr(JsonReader) reader = json_reader_new(json_parser_get_root(parser));
    
    if (!json_reader_read_member(reader, "music_data")) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, 
                   "Failed to read 'music_data' member");
        return NULL;
    }
    
    guint n_lists = json_reader_count_elements(reader);
    if (n_lists == 0) {
        json_reader_end_member(reader);
        return NULL;
    }
    
    GList *music_lists = NULL;
    
    for (guint i = 0; i < n_lists; i++) {
        json_reader_read_element(reader, i);
        
        MusicListData *list_data = g_new0(MusicListData, 1);
        
        if (json_reader_read_member(reader, "list_box_name")) {
            const gchar *name = json_reader_get_string_value(reader);
            if(!name){
                log_error("No list_box_name found at line %d", __LINE__);
                json_reader_end_member(reader);
                continue;
            }

            list_data->name = g_strdup(name);

            json_reader_end_member(reader);
        }
        
        if (json_reader_read_member(reader, "music_list_path")) {
            guint n_paths = json_reader_count_elements(reader);

            for (guint j = 0; j < n_paths; j++) {
                MusicDataSave *music_data = g_new0(MusicDataSave, 1);
                json_reader_read_element(reader, j);
                json_reader_read_member(reader, "music_path");
                const gchar *path = json_reader_get_string_value(reader);
                json_reader_end_member(reader);

                json_reader_read_member(reader, "music_duration");
                const gchar *duration = json_reader_get_string_value(reader);
                json_reader_end_member(reader);
                if(!path || !duration){
                    log_error("Failed to read 'music_path' or 'music_duration' member");
                    json_reader_end_element(reader);
                    g_free(music_data);
                    continue;
                }
                char *full_path_analyze = g_strconcat(SYM_AUDIO_DIR, path, NULL);
                if(!g_file_test(full_path_analyze, G_FILE_TEST_EXISTS)){
                    log_error("Music %s not found", full_path_analyze);
                    g_free(full_path_analyze);
                    json_reader_end_element(reader);
                    g_free(music_data);
                    continue;
                }

                music_data->music_path = g_strdup(path);
                music_data->music_duration = g_strdup(duration);

                list_data->music_list = g_list_append(list_data->music_list, music_data);

                json_reader_end_element(reader);
            }
            
            json_reader_end_member(reader);

        }
        
        if(g_list_length(list_data->music_list) == 0){
            g_list_free(list_data->music_list);
            g_free(list_data->name);
            g_free(list_data);
            continue;
        }
        
        music_lists = g_list_append(music_lists, list_data);
        
        json_reader_end_element(reader);
    }

    GHashTable *valid_paths = g_hash_table_new_full (g_str_hash,
                                                 g_str_equal,
                                                 g_free,   
                                                 NULL);   

    for (GList *l = music_lists; l; l = l->next) {
        MusicListData *list_data = l->data;
        for (GList *m = list_data->music_list; m; m = m->next) {
            MusicDataSave *md = m->data;
            g_hash_table_add (valid_paths, g_strdup (md->music_path));
        }
    }

    for (GList *s = symlinks_musics_to_analyze; s; s = s->next) {
        const char *music_path = s->data;        
        if (!g_hash_table_contains (valid_paths, music_path)) {
            char *symlink = g_strconcat (SYM_AUDIO_DIR, music_path, NULL);
            log_warning ("Removing symlink %s", symlink);
            g_unlink (symlink);
            g_free (symlink);
        }


    }

    g_hash_table_destroy (valid_paths);
    
    
    json_reader_end_member(reader);
    return music_lists;
}

static GtkWidget*
create_music_row(const gchar *music_path, const gchar *music_duration, EuteranMainObject *self, GtkWidget *list_box)
{
    if (!music_path) {
        log_error("Music path is null");
        return NULL;
    }
    
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box_music_name_and_duration = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    gtk_widget_set_hexpand(box_music_name_and_duration, TRUE);
    gtk_widget_set_margin_top(box_music_name_and_duration, 0);
    gtk_widget_set_margin_bottom(box_music_name_and_duration, 0);
    
    EuteranDialogContainer *music_container = g_new0(EuteranDialogContainer, 1);
    gchar *music_full_path = g_strconcat(SYM_AUDIO_DIR, music_path, NULL);
    
    music_container->window_parent = GTK_WIDGET(euteran_main_object_get_widget_at(self, WINDOW_PARENT));
    music_container->row = row;
    music_container->row_box =box_music_name_and_duration;
    music_container->list_box = list_box;
    music_container->path = g_strdup(music_full_path);

    GtkGesture *event_mouse = gtk_gesture_click_new();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE(event_mouse), 3);

    gtk_widget_add_controller(music_container->row_box, GTK_EVENT_CONTROLLER(event_mouse));
    g_signal_connect(event_mouse, "pressed", G_CALLBACK(on_pressed_right_click_event), music_container);
    
    GtkWidget *label = gtk_label_new(music_path);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(label), 1.0);
    gtk_widget_set_halign(label, GTK_ALIGN_BASELINE_CENTER);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_box_append(GTK_BOX(box_music_name_and_duration), label);
    
    GtkWidget *separator_line = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(separator_line, 1, -1);
    gtk_widget_set_halign(separator_line, GTK_ALIGN_END);
    gtk_widget_add_css_class(separator_line, "line_class");
    gtk_box_append(GTK_BOX(box_music_name_and_duration), separator_line);
    
    double duration_value = formatted_string_to_double(music_duration);

    const char *duration_str = music_duration;
    GtkWidget *duration = gtk_label_new(duration_str ? duration_str : "0.0");
    gtk_widget_set_size_request(duration, 75, -1);
    gtk_label_set_ellipsize(GTK_LABEL(duration), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(duration), 0.0);
    gtk_box_append(GTK_BOX(box_music_name_and_duration), duration);

    
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box_music_name_and_duration);
    
    g_object_set_data(G_OBJECT(row), "music_duration", cast_simple_double_to_string(duration_value));
    
    g_free(music_full_path);
    return row;
}

static GtkWidget*
create_new_stack_page(GtkWidget *stack, const gchar *page_name)
{
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled_window), 297);
    
    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_BROWSE);
    gtk_widget_add_css_class(list_box, "list_box");
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), list_box);
    
    GtkStackPage *stack_page = gtk_stack_add_child(GTK_STACK(stack), scrolled_window);
    gtk_stack_page_set_title(GTK_STACK_PAGE(stack_page), page_name);
    
    return list_box;
}

static void
populate_music_list(GtkWidget *list_box, const MusicListData *list_data, 
                   EuteranMainObject *self, PlayMusicFunc play_music_func)
{
    for(GList *data = list_data->music_list; data != NULL; data = data->next) {
        MusicDataSave *musics = data->data;
        
        if (!musics) {
            log_error("Music data is null");
            continue;
        }

        const gchar *music_duration = musics->music_duration;
        const gchar *music_path = musics->music_path;
        

        GtkWidget *row = create_music_row(music_path, music_duration, self, list_box);
        if (row) {
            gtk_list_box_insert(GTK_LIST_BOX(list_box), row, -1);
        }
    }
    
    g_signal_handlers_disconnect_by_func(list_box, G_CALLBACK(play_music_func), self);
    g_signal_connect(list_box, "row-activated", G_CALLBACK(play_music_func), self);
}

static void
process_music_list(MusicListData *list_data, guint index, GtkWidget *stack, 
                  GtkSelectionModel *pages, guint n_existing_pages,
                  EuteranMainObject *self, PlayMusicFunc play_music_func)
{
    GtkWidget *list_box = NULL;
    
    if (index < n_existing_pages) {
        list_box = euteran_main_object_get_list_box(pages, index);
    }
    
    if (!list_box || !GTK_IS_LIST_BOX(list_box)) {
        list_box = create_new_stack_page(stack, list_data->name);
    }
    
    populate_music_list(list_box, list_data, self, play_music_func);
}


static GList *
get_symlinks_musics_to_analyze(void) 
{
    GList *symlinks = NULL;
    DIR *dir = opendir(SYM_AUDIO_DIR);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_LNK) {
                symlinks = g_list_append(symlinks, g_strdup(entry->d_name));
            }
        }
        closedir(dir);
    }
    return symlinks;
}

void 
euteran_main_object_load_and_apply_data_json(EuteranMainObject *self, 
                                            PlayMusicFunc play_music_func)
{
    g_return_if_fail(EUTERAN_IS_MAIN_OBJECT(self));
    
    g_autofree gchar *json_path = g_build_filename(CONFIGURATION_DIR, "music_data.json", NULL);
    g_autoptr(GError) error = NULL;
    GList *symlinks_musics_to_analyze = get_symlinks_musics_to_analyze();
    GList *music_lists = load_music_data_from_json(json_path, &error, symlinks_musics_to_analyze);
    
    if (error) {
        g_print("Error loading music data: %s\n", error->message);
        return;
    }
    
    if (!music_lists) {
        log_info("No music data found");
        return;
    }
    
    GtkWidget *stack = (GtkWidget *)euteran_main_object_get_widget_at(self, STACK);
    GtkSelectionModel *pages = gtk_stack_get_pages(GTK_STACK(stack));
    guint n_existing_pages = g_list_model_get_n_items(G_LIST_MODEL(pages));
    
    guint index = 0;
    for (GList *l = music_lists; l != NULL; l = l->next) {
        MusicListData *list_data = (MusicListData *)l->data;
        process_music_list(list_data, index, stack, pages, n_existing_pages, 
                          self, play_music_func);
        index++;
    }
    
    g_list_free(symlinks_musics_to_analyze);
    g_list_free_full(music_lists, (GDestroyNotify)free_music_list_data);
}


/* vou modularizar isso depois, por enquanto vou deixar aqui */
