#include "eut_binaryparser.h"
#include "eut_constants.h"
#include "eut_logs.h"
#include "glib.h"
#include "glib/gstdio.h"
#include "eut_widgetsfunctions.h"
#include "eut_musiclistfunc.h"

struct _EutBinaryParser{
    GObject parent_instance;
    GHashTable* Music_references;
};

G_DEFINE_FINAL_TYPE(EutBinaryParser, eut_binary_parser, G_TYPE_OBJECT)

static void 
eut_binary_parser_init(EutBinaryParser *self) {
    self->Music_references = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

static void 
eut_binary_parser_dispose(GObject *object)
{
    G_OBJECT_CLASS(eut_binary_parser_parent_class)->dispose(object);
}

static void 
eut_binary_parser_class_init(EutBinaryParserClass *klass) {
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = eut_binary_parser_dispose;
}

EutBinaryParser*
eut_binary_parser_new(void) {
    return g_object_new(TYPE_EUT_BINARY_PARSER, nullptr);
}

void
eut_binary_parser_save_binary(EutBinaryParser *self, void *main_object)
{
    EuteranMainObject *mb = (EuteranMainObject *)main_object;
    g_return_if_fail (EUTERAN_IS_MAIN_OBJECT (mb));

    GtkWidget        * stack  = (GtkWidget *)euteran_main_object_get_stack(mb);
    GtkSelectionModel* pages  = gtk_stack_get_pages (GTK_STACK (stack));
    guint             n_items = g_list_model_get_n_items (G_LIST_MODEL (pages));

    if (n_items == 0) return;

    char *full_path = malloc(strlen(CONFIGURATION_DIR) + strlen("music_data.dat") + 1);
    sprintf(full_path, "%s%s", CONFIGURATION_DIR, "music_data.dat");
    
    FILE *binary_file_to_save = fopen(full_path, "wb");

    if(!binary_file_to_save){
        log_error("Failed to open file to save music data");
        return;
    }

    free(full_path);

    uint32_t n_items_32 = GINT32_TO_LE(n_items);
    fwrite(&n_items_32, sizeof(uint32_t), 1, binary_file_to_save);
    

    for(guint i = 0; i < n_items; i++){
        GtkStackPage *page = g_list_model_get_item(G_LIST_MODEL(pages), i);
        if(!page || !GTK_IS_STACK_PAGE(page)){
            continue;
        }

        const char *list_box_name = gtk_stack_page_get_title(page);
        if(!list_box_name){
            continue;
        }

        GtkWidget *scrolled_window = gtk_stack_page_get_child(page);
        if(!scrolled_window || !GTK_IS_SCROLLED_WINDOW(scrolled_window)){
            continue;
        }

        GtkWidget *list_box = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled_window));
        if(!list_box){
            continue;
        }

        if(GTK_IS_VIEWPORT(list_box)){
            list_box = gtk_viewport_get_child(GTK_VIEWPORT(list_box));
        }

        if(!list_box || !GTK_IS_LIST_BOX(list_box)){
            continue;
        }
        
        size_t list_box_name_len_ = strlen(list_box_name);
        uint16_t list_box_name_len = GINT16_TO_LE(list_box_name_len_);
        fwrite(&list_box_name_len, sizeof(uint16_t), 1, binary_file_to_save);
        fwrite(list_box_name, list_box_name_len_, 1, binary_file_to_save);

        gint music_count = 0;
        while(gtk_list_box_get_row_at_index(GTK_LIST_BOX(list_box), music_count) != nullptr){
            music_count++;
        }
        uint32_t music_count_items = GINT32_TO_LE(music_count);
        fwrite(&music_count_items, sizeof(uint32_t), 1, binary_file_to_save);

        for(GtkWidget *row = gtk_widget_get_first_child(GTK_WIDGET(list_box)); row != nullptr; row = gtk_widget_get_next_sibling(row)){
            GtkWidget *row_box = gtk_widget_get_first_child(GTK_WIDGET(row));
            if(!row_box || !GTK_IS_BOX(row_box)){
                continue;
            }

            GtkWidget *label = gtk_widget_get_first_child (row_box);
            GtkWidget *music_duration = gtk_widget_get_last_child (row_box);

            if(!label || !music_duration || !GTK_IS_LABEL(label) || !GTK_IS_LABEL(music_duration)){
                continue;
            }

            const gchar *music_name = gtk_label_get_text (GTK_LABEL (label));
            const gchar *music_duration_text = gtk_label_get_text (GTK_LABEL (music_duration));
            const gchar *music_duration_raw = g_object_get_data (G_OBJECT(row), "music_duration");

            if(!music_name || !music_duration_text || !music_duration_raw){
                continue;
            }

            size_t music_name_len_ = strlen(music_name);
            uint16_t music_name_len = GINT16_TO_LE(music_name_len_);
            fwrite(&music_name_len, sizeof(uint16_t), 1, binary_file_to_save);
            fwrite(music_name, music_name_len_, 1, binary_file_to_save);
            
            size_t music_duration_len_ = strlen(music_duration_text);
            uint16_t music_duration_len = GINT16_TO_LE(music_duration_len_);
            fwrite(&music_duration_len, sizeof(uint16_t), 1, binary_file_to_save);
            fwrite(music_duration_text, music_duration_len_, 1, binary_file_to_save);
            
            size_t music_duration_raw_len_ = strlen(music_duration_raw);
            uint16_t music_duration_raw_len = GINT16_TO_LE(music_duration_raw_len_);
            fwrite(&music_duration_raw_len, sizeof(uint16_t), 1, binary_file_to_save);
            fwrite(music_duration_raw, music_duration_raw_len_, 1, binary_file_to_save);
        }
    }

    fclose(binary_file_to_save);
    log_info("Saved music data");
}

static void
free_music_list_data(ParsedBinaryData *data)
{
    if (!data) return;
    
    g_free(data->name);
    g_list_free(data->music_list);
    g_free(data);
}

static GList*
eut_binary_parser_get_parsed_music_data(
    EutBinaryParser    *self, 
    const gchar        *binary_path, 
    GError             **error, 
    GList              *symlinks_musics_to_analyze
)
{       
    FILE *binary_to_read = fopen(binary_path, "rb");

    if(binary_to_read == nullptr){
        return nullptr;
    }

    GList *music_list = nullptr;

    uint32_t n_items_32_raw;
    fread(&n_items_32_raw, sizeof(uint32_t), 1, binary_to_read);
    uint32_t n_items_32 = GUINT32_FROM_LE(n_items_32_raw);

    if(n_items_32 <= 0){
        return nullptr;
    }

    char *last_music = nullptr;

    for(uint32_t i = 0; i < n_items_32; i++){
        ParsedBinaryData *list_data = g_new0(ParsedBinaryData, 1);
        uint16_t list_box_name_len_raw;
        fread(&list_box_name_len_raw, sizeof(uint16_t), 1, binary_to_read);
        uint16_t list_box_name_len = GUINT16_FROM_LE(list_box_name_len_raw);
        char *list_box_name = malloc(list_box_name_len + 1);
        fread(list_box_name, list_box_name_len, 1, binary_to_read);
        
        if(list_box_name == nullptr){
            continue;
        }

        list_box_name[list_box_name_len] = '\0';

        list_data->name = list_box_name;

        uint32_t n_musics_32_raw;
        fread(&n_musics_32_raw, sizeof(uint32_t), 1, binary_to_read);
        uint32_t n_musics_32 = GUINT32_FROM_LE(n_musics_32_raw);

        for(uint32_t j = 0; j < n_musics_32; j++){
            MusicDataInformation *music_data = g_new0(MusicDataInformation, 1);
            uint16_t music_path_len_raw;
            fread(&music_path_len_raw, sizeof(uint16_t), 1, binary_to_read);
            uint16_t music_path_len = GUINT16_FROM_LE(music_path_len_raw);
            char *music_path = malloc(music_path_len + 1);
            fread(music_path, music_path_len, 1, binary_to_read);

            if(music_path == nullptr){
                continue;
            }

            music_path[music_path_len] = '\0';

            uint16_t music_duration_len_raw;
            fread(&music_duration_len_raw, sizeof(uint16_t), 1, binary_to_read);
            uint16_t music_duration_len = GUINT16_FROM_LE(music_duration_len_raw);
            char *music_duration = malloc(music_duration_len + 1);
            fread(music_duration, music_duration_len, 1, binary_to_read);
    
            if(music_duration == nullptr){
                continue;
            }

            music_duration[music_duration_len] = '\0';

            uint16_t music_duration_raw_len_raw;
            fread(&music_duration_raw_len_raw, sizeof(uint16_t), 1, binary_to_read);
            uint16_t music_duration_raw_len = GUINT16_FROM_LE(music_duration_raw_len_raw);
            char *music_duration_raw = malloc(music_duration_raw_len + 1);
            fread(music_duration_raw, music_duration_raw_len, 1, binary_to_read);

            if(music_duration_raw == nullptr){
                continue;
            }

            music_duration_raw[music_duration_raw_len] = '\0';

            char *full_path_analyze = g_strconcat(SYM_AUDIO_DIR, music_path, nullptr);
            if(!g_file_test(full_path_analyze, G_FILE_TEST_EXISTS)){
                    log_error("Music %s not found", full_path_analyze);
                    g_free(full_path_analyze);
                    g_free(music_path);
                    g_free(music_duration);
                    g_free(music_duration_raw);
                    g_free(music_data);
                    continue;
            }


            if(g_hash_table_lookup(self->Music_references, music_path) == nullptr){
                g_hash_table_insert(self->Music_references, g_strdup(music_path), g_strdup("1"));
            } else {
                char *reference = g_hash_table_lookup(self->Music_references, music_path);
                long num = strtol(reference, nullptr, 10);
                char *num_str = g_strdup_printf("%d", (int)num + 1);
                g_hash_table_replace(self->Music_references, g_strdup(music_path), num_str);
            }


            music_data->music_path = music_path;
            music_data->music_duration = music_duration;
            music_data->music_duration_raw = music_duration_raw;



            list_data->music_list = g_list_append(list_data->music_list, music_data);
        }

        if(list_data->music_list == nullptr || g_list_length(list_data->music_list) == 0){
            g_free(list_box_name);
            g_free(list_data);
            continue;
        }
        music_list = g_list_append(music_list, list_data);
    }

    fclose(binary_to_read);
    GHashTable* valid_paths = g_hash_table_new_full (g_str_hash,
                                                 g_str_equal,
                                                 g_free,   
                                                 nullptr);   

    for (GList* l = music_list; l; l = l->next) {
        ParsedBinaryData* list_data = l->data;
        for (GList* m = list_data->music_list; m; m = m->next) {
            MusicDataInformation* md = m->data;
            g_hash_table_add (valid_paths, g_strdup (md->music_path));
        }
    }

    for (GList *s = symlinks_musics_to_analyze; s; s = s->next) {
        const char* music_path = s->data;        
        if (!g_hash_table_contains (valid_paths, music_path)) {
            char* symlink = g_strconcat (SYM_AUDIO_DIR, music_path, nullptr);
            log_warning ("Removing symlink %s", symlink);
            g_unlink (symlink);
            g_free (symlink);
        }
    }

    g_hash_table_destroy (valid_paths);
    return music_list;
}

static GtkWidget*
create_new_stack_page(GtkWidget *stack, const gchar *page_name)
{
    GtkWidget* scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    
    GtkWidget* list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_SINGLE);
    gtk_widget_add_css_class(list_box, "list_box");
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), list_box);
    
    GtkStackPage* stack_page = gtk_stack_add_child(GTK_STACK(stack), scrolled_window);
    gtk_stack_page_set_title(GTK_STACK_PAGE(stack_page), page_name);
    
    return list_box;
}

static void
populate_music_list(GtkWidget *list_box, const ParsedBinaryData *list_data, 
                   EuteranMainObject *self)
{
    for(GList* data = list_data->music_list; data != nullptr; data = data->next) {
        MusicDataInformation* musics = data->data;
        if (!musics) {
            log_error("Music data is null");
            continue;
        }

        const gchar *music_duration = musics->music_duration;
        const gchar *music_path = musics->music_path;
        const gchar *music_duration_raw = musics->music_duration_raw;       
        
        char* endptr;
        double duration = 0;
        duration = strtod(music_duration_raw, &endptr);


        create_music_row(self, GTK_LIST_BOX(list_box) , music_path, music_duration_raw, duration);
    }
    
    g_signal_handlers_disconnect_by_func(list_box, G_CALLBACK(play_selected_music), self);
    g_signal_connect(list_box, "row-activated", G_CALLBACK(play_selected_music), self);
}

static void
process_music_list(ParsedBinaryData *list_data, guint index, GtkWidget *stack, 
                  GtkSelectionModel *pages, guint n_existing_pages,
                  EuteranMainObject *self)
{
    GtkWidget *list_box = nullptr;
    
    if (index < n_existing_pages) {
        list_box = euteran_main_object_get_index_list_box(pages, index);
    }
    
    if (!list_box || !GTK_IS_LIST_BOX(list_box)) {
        list_box = create_new_stack_page(stack, list_data->name);
    }
    
    populate_music_list(list_box, list_data, self);
}

static GList*
get_symlinks_musics_to_analyze(void) 
{
    GList *symlinks = nullptr;
    DIR *dir = opendir(SYM_AUDIO_DIR);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_LNK) {
                symlinks = g_list_append(symlinks, g_strdup(entry->d_name));
            }
        }
        closedir(dir);
    }
    return symlinks;

}

void 
eut_binary_parser_load_and_apply_binary
(
    EutBinaryParser* self,
    void*            main_object
)
{
    EuteranMainObject* mb = main_object;
    g_return_if_fail(EUTERAN_IS_MAIN_OBJECT(mb));

    g_autofree gchar *binary_path = g_build_filename(CONFIGURATION_DIR, "music_data.dat", nullptr);
    g_autoptr(GError) error = nullptr;
    GList *symlinks_musics_to_analyze = get_symlinks_musics_to_analyze();
    GList *music_lists = eut_binary_parser_get_parsed_music_data(self, binary_path, &error, symlinks_musics_to_analyze);
    
    if (error) {
        g_print("Error loading music data: %s\n", error->message);
        return;
    }
    
    if (!music_lists) {
        log_info("No music data found");
        return;
    }
    
    GtkWidget* stack = (GtkWidget*)euteran_main_object_get_stack(mb);
    GtkSelectionModel* pages = gtk_stack_get_pages(GTK_STACK(stack));
    guint n_existing_pages = g_list_model_get_n_items(G_LIST_MODEL(pages));
    
    guint index = 0;
    for (GList *l = music_lists; l != nullptr; l = l->next) {
        ParsedBinaryData *list_data = (ParsedBinaryData *)l->data;
        process_music_list(list_data, index, stack, pages, n_existing_pages, 
                          mb);
        index++;
    }
    
    g_list_free(symlinks_musics_to_analyze);
    g_list_free_full(music_lists, (GDestroyNotify)free_music_list_data);
}
