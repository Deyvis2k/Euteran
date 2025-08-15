#include "eut_main_object.h"
#include "eut_foldercb.h"
#include "eut_logs.h"
#include "eut_settings.h"
#include "eut_subwindowimpl.h"
#include "gdk/gdk.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "eut_utils.h"
#include "eut_subwindow.h"
#include "eut_widgetsfunctions.h"
#include "eut_binaryparser.h"

struct _EuteranMainObject {
    AdwApplicationWindow parent_instance;

    double music_duration;
    double offset_time;
    void *optional_object;

    GtkGrid* music_display_content;
    GtkScale* progress_bar;
    GtkScale* volume_slider;
    GtkButton* music_button;
    GtkListBox* list_box;
    GtkMenuButton* menu_button;
    GtkStack* view_stack_main;
    GtkStackSwitcher* stack_switcher;
    GtkButton* switcher_add_button;
    GtkButton* stop_button;
    GtkScale* input_slider;
    GtkButton* stop_recording_button;
    GtkButton* alter_theme_button;
    GtkButton* button_input_recording;
    GtkButton* button_input_stop_recording;

    GtkGestureClick* gesture_click;
    GtkDropTarget* drop_target;

    EuteranThemeData* euteran_theme;
    EuteranSettings* settings;
    EutSubwindow* subwindow;
    EutBinaryParser* binary_parser;
    
    GTimer *timer;
    GCancellable* current_cancellable;
};

G_DEFINE_FINAL_TYPE(EuteranMainObject, euteran_main_object, ADW_TYPE_APPLICATION_WINDOW);

static void 
euteran_main_object_dispose(GObject *object) {
    EuteranMainObject *self = EUTERAN_MAIN_OBJECT(object);

    log_warning("Cleaning Euteran Main Object");

    if (self->timer != nullptr) {
        g_timer_destroy(self->timer);
        self->timer = nullptr;
    }
    g_object_unref(self->euteran_theme->provider);
    g_object_unref(self->euteran_theme->theme_settings);
    g_object_unref(self->subwindow);
    g_object_unref(self->binary_parser);
    g_free(self->euteran_theme);
    self->optional_object = nullptr;
    self->music_duration = 0.0;
    self->offset_time = 0.0;

    G_OBJECT_CLASS(euteran_main_object_parent_class)->dispose(object);
}



static void 
euteran_main_object_class_init(EuteranMainObjectClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    object_class->dispose = euteran_main_object_dispose;


    gtk_widget_class_set_template_from_resource(widget_class, "/org/euteran/euteran_main.ui");

    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, music_display_content);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, progress_bar);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, volume_slider);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, music_button);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, list_box);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, menu_button);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, view_stack_main);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, stack_switcher);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, switcher_add_button);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, stop_button);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, input_slider);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, stop_recording_button);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, alter_theme_button);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, button_input_recording);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, gesture_click);
    gtk_widget_class_bind_template_child(widget_class, EuteranMainObject, drop_target);


    gtk_widget_class_bind_template_callback(widget_class, select_folder);
    gtk_widget_class_bind_template_callback(widget_class, setup_subwindow);
    gtk_widget_class_bind_template_callback(widget_class, euteran_main_object_switch_theme);
    gtk_widget_class_bind_template_callback(widget_class, start_recording_input);
    gtk_widget_class_bind_template_callback(widget_class, stop_audio_input_recording);
    gtk_widget_class_bind_template_callback(widget_class, on_clicked_input_slider);
    gtk_widget_class_bind_template_callback(widget_class, on_volume_changed);
    gtk_widget_class_bind_template_callback(widget_class, on_clicked_progress_bar);
    gtk_widget_class_bind_template_callback(widget_class, pause_audio);
    gtk_widget_class_bind_template_callback(widget_class, interrupt_audio);
    gtk_widget_class_bind_template_callback(widget_class, on_window_destroy);
    gtk_widget_class_bind_template_callback(widget_class, on_switcher_add_button);
    gtk_widget_class_bind_template_callback(widget_class, on_stack_switcher_right_click);
    gtk_widget_class_bind_template_callback(widget_class, on_drop);
}

static void 
euteran_main_object_init(EuteranMainObject *self) {

    gtk_widget_init_template(GTK_WIDGET(self));
    self->music_duration = 0.0;
    self->timer = g_timer_new();
    self->euteran_theme = g_new0(EuteranThemeData, 1);
    self->euteran_theme->theme = EUTERAN_DARKMODE;
    self->euteran_theme->theme_settings = gtk_settings_get_default();
    self->euteran_theme->provider = gtk_css_provider_new();
    self->settings = euteran_settings_new();
    self->offset_time = 0.0;
    self->optional_object = nullptr;
    self->current_cancellable = nullptr;
    self->subwindow = eut_subwindow_new();
    eut_subwindow_set_settings(self->subwindow, self->euteran_theme->theme_settings);
    self->binary_parser = eut_binary_parser_new();

    gtk_css_provider_load_from_path(self->euteran_theme->provider, "Style/dark/style.css");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                              GTK_STYLE_PROVIDER(self->euteran_theme->provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);
    
    gtk_range_set_value(GTK_RANGE(self->volume_slider), euteran_settings_get_last_volume(self->settings));




    gtk_window_set_default_size(
        GTK_WINDOW(self),
        euteran_settings_get_window_width(self->settings),
        euteran_settings_get_window_height(self->settings)
    );

    gtk_drop_target_set_gtypes(self->drop_target, (GType[1]) { GDK_TYPE_FILE_LIST, }, 1);
    eut_binary_parser_load_and_apply_binary(self->binary_parser, self);
}

GtkCssProvider *
euteran_main_object_get_css_provider(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), nullptr);
    return self->euteran_theme->provider;
}

EuteranThemeData *
euteran_main_object_get_theme_data(EuteranMainObject *self)
{
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), nullptr);
    return self->euteran_theme;
}

GtkSettings *
euteran_main_object_get_theme_settings(EuteranMainObject *self) 
{
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), nullptr);

    return self->euteran_theme->theme_settings;    
}

EutSubwindow * 
euteran_main_object_get_subwindow(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), nullptr);
    return self->subwindow;
}

EuteranSettings *
euteran_main_object_get_settings_object(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), nullptr);
    return self->settings;
}

void 
euteran_main_object_switch_theme(EuteranMainObject *self, GtkWidget *button)
{
    g_return_if_fail(EUTERAN_IS_MAIN_OBJECT(self));

    gtk_style_context_remove_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(self->euteran_theme->provider)); 
    switch(self->euteran_theme->theme) {
        case EUTERAN_DARKMODE:
            char *abs_path = get_absolute_path(STYLE_FILE);
            gtk_css_provider_load_from_path(self->euteran_theme->provider, g_build_filename(abs_path, "/light/style.css", nullptr));
            g_free(abs_path);
            self->euteran_theme->theme = EUTERAN_LIGHTMODE;
            gtk_button_set_label(self->alter_theme_button, "DarkMode");
            break;
        case EUTERAN_LIGHTMODE:
            char *abs_path_ = get_absolute_path(STYLE_FILE);
            gtk_css_provider_load_from_path(self->euteran_theme->provider, g_build_filename(abs_path_, "/dark/style.css", nullptr));
            g_free(abs_path_);
            self->euteran_theme->theme = EUTERAN_DARKMODE;
            gtk_button_set_label(self->alter_theme_button, "LightMode");
            break;
        default:
            break;
    }
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                              GTK_STYLE_PROVIDER(self->euteran_theme->provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);
}

EuteranMainObject *
euteran_main_object_new(void) {
    return g_object_new(TYPE_EUTERAN_MAIN_OBJECT, nullptr);
}


void 
euteran_main_object_set_duration(EuteranMainObject *self, double duration) {
    self->music_duration = duration;
}

void 
euteran_main_object_set_timer(EuteranMainObject *self, GTimer *timer) {
    g_return_if_fail(EUTERAN_IS_MAIN_OBJECT(self));

    if (self->timer != nullptr) {
        g_timer_destroy(self->timer);
    }

    self->timer = timer;
}

void 
euteran_main_object_set_offset_time(EuteranMainObject *self, double offset_time) {
    self->offset_time = offset_time;
}

double 
euteran_main_object_get_offset_time(EuteranMainObject *self) {
    return self->offset_time;
}

double 
euteran_main_object_get_duration(EuteranMainObject *self) {
    return self->music_duration;
}

GTimer *
euteran_main_object_get_timer(EuteranMainObject *self) {
    return self->timer;
}

void 
euteran_main_object_set_optional_pointer_object(EuteranMainObject *self, void *optional_pointer_object) {
    self->optional_object = optional_pointer_object;
}

void *
euteran_main_object_get_optional_pointer_object(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), nullptr);

    return self->optional_object != nullptr ? self->optional_object : nullptr;
}


void *
euteran_main_object_get_binary_parser(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), nullptr);
    return self->binary_parser;
}

void
euteran_main_object_reset_cancellable(EuteranMainObject *self) {
    g_return_if_fail(EUTERAN_IS_MAIN_OBJECT(self));
    if (self->current_cancellable) {
        g_cancellable_cancel(self->current_cancellable);
        g_clear_object(&self->current_cancellable);
    }
    self->current_cancellable = g_cancellable_new();
}


GtkGrid* euteran_main_object_get_music_display_content(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->music_display_content;
}

GtkScale* euteran_main_object_get_progress_bar(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->progress_bar;
}

GtkScale* euteran_main_object_get_volume_slider(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->volume_slider;
}

GtkButton* euteran_main_object_get_music_button(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->music_button;
}

GtkListBox* euteran_main_object_get_list_box(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->list_box;
}

GtkMenuButton* euteran_main_object_get_menu_button(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->menu_button;
}

GtkStack* euteran_main_object_get_stack(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->view_stack_main;
}

GtkStackSwitcher* euteran_main_object_get_stack_switcher(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->stack_switcher;
}

GtkButton* euteran_main_object_get_switcher_add_button(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->switcher_add_button;
}

GtkButton* euteran_main_object_get_stop_button(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->stop_button;
}

GtkScale* euteran_main_object_get_input_slider(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->input_slider;
}

GtkButton* euteran_main_object_get_input_button(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->button_input_recording;
}

GtkButton* euteran_main_object_get_stop_recording_button(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->stop_recording_button;
}

GtkButton* euteran_main_object_get_alter_theme_button(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->alter_theme_button;
}

GtkButton* euteran_main_object_get_button_input_recording(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->button_input_recording;
}

GtkButton* euteran_main_object_get_button_input_stop_recording(EuteranMainObject *self) {
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), NULL);
    return self->button_input_stop_recording;
}

GtkWidget*
euteran_main_object_get_index_list_box
(
    GtkSelectionModel *pages,
    guint             index
)
{
    GtkWidget *list_box = nullptr;

    GtkStackPage *page = g_list_model_get_item (G_LIST_MODEL (pages), index);
    if(!page || !GTK_IS_STACK_PAGE(page)){
        return nullptr;
    }

    GtkWidget *scrolled_window = gtk_stack_page_get_child (page);
    if(!scrolled_window || !GTK_IS_SCROLLED_WINDOW(scrolled_window)){
        return nullptr;
    }

    list_box = gtk_scrolled_window_get_child (GTK_SCROLLED_WINDOW (scrolled_window));
    if(!list_box){
        return nullptr;
    }

    if(GTK_IS_VIEWPORT(list_box)){
        list_box = gtk_viewport_get_child (GTK_VIEWPORT (list_box));
    }

    if(!GTK_IS_LIST_BOX(list_box)){
        return nullptr;
    }


    return list_box;
}

GtkListBox *
euteran_main_object_get_focused_list_box
(
    EuteranMainObject *self
)
{
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), nullptr);

    GtkWidget *visible_child = gtk_stack_get_visible_child(self->view_stack_main);
    if(!visible_child || !GTK_IS_SCROLLED_WINDOW(visible_child))
        return nullptr;

    GtkWidget *list_box = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(visible_child)); 

    if(GTK_IS_VIEWPORT(list_box)) {
        list_box = gtk_viewport_get_child(GTK_VIEWPORT(list_box));
    } 

    if(!list_box || !GTK_IS_LIST_BOX(list_box))
        return nullptr;
    
    return GTK_LIST_BOX(list_box);
}

gboolean euteran_main_object_list_box_contains_music(EuteranMainObject *self, const gchar *music_to_analyze)
{
    g_return_val_if_fail(EUTERAN_IS_MAIN_OBJECT(self), FALSE);

    GtkListBox *list_box = euteran_main_object_get_focused_list_box(self);
    if(!list_box || !GTK_IS_LIST_BOX(list_box))
        return FALSE;

    GtkListBoxRow *child;
    for(int i = 0; (child = gtk_list_box_get_row_at_index(list_box, i)); i++) {
        GtkWidget *row_box = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(child));
        GtkWidget *label_to_analyze = gtk_widget_get_first_child(row_box);

        if(g_strcmp0(gtk_label_get_text(GTK_LABEL(label_to_analyze)), music_to_analyze) == 0)
            return TRUE;
    }

    return FALSE;
}

GCancellable *
euteran_main_object_get_cancellable(EuteranMainObject *self) {
    return self->current_cancellable;
}

void 
euteran_main_object_set_cancellable(
    EuteranMainObject *self, 
    GCancellable *cancellable
) 
{
    self->current_cancellable = cancellable;
}
