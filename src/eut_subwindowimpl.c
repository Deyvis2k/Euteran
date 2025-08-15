#include "eut_subwindowimpl.h"
#include "eut_main_object.h"
#include "eut_logs.h"
#include "eut_subwindow.h"

gboolean
on_dropdown_theme_selected 
(
    EutSubwindow *sub_window,
    GParamSpec *pspec,
    GObject    *obj
)
{

    static gchar last_theme[1024] = "";

    GtkSettings *theme_settings = eut_subwindow_get_main_object_settings(sub_window);
    
    gpointer *item = gtk_drop_down_get_selected_item(GTK_DROP_DOWN(obj));

    if(item == NULL){
        log_error("item is null");
        return FALSE;
    }

    GtkStringObject *string_object = (GtkStringObject *)item;
    if(!GTK_IS_STRING_OBJECT(string_object)){
        log_error("item is not a GtkStringObject");
        return FALSE;
    }

    const char *name = gtk_string_object_get_string(string_object);

    if(g_strcmp0(last_theme, name) == 0){
        log_info("Theme already selected: %s", name);
        return FALSE;
    }
    g_strlcpy(last_theme, name, sizeof(last_theme));

    if(name == NULL){
        log_error("name is null");
        return FALSE;
    }

    if(strcmp(name, "Adwaita") != 0)g_object_set(theme_settings, "gtk_theme_name", name, NULL);
    else g_object_set(theme_settings, "gtk_theme_name", "Adwaita-empty", NULL);

    return TRUE;
}

gboolean
on_dropdown_icon_selected
(
    EutSubwindow *sub_window,
    GParamSpec *pspec,
    GObject    *obj
)
{
    gpointer *item = gtk_drop_down_get_selected_item(GTK_DROP_DOWN(obj));
    GtkStringObject *string_object = (GtkStringObject *)item;

    const char *name = gtk_string_object_get_string(string_object);
    
    GtkSettings *theme_settings = eut_subwindow_get_main_object_settings(sub_window);

    
    g_object_set(theme_settings, "gtk_icon_theme_name", name, NULL);

    return TRUE;
}


static gboolean 
on_window_hide_request
(
    GtkWidget   *window, 
    gpointer    user_data
) 
{
    EutSubwindow *sub_window = EUT_SUBWINDOW(user_data);
    eut_subwindow_set_allocated_window_size(sub_window);

    eut_subwindow_set_window_visibility(sub_window, FALSE);
    return TRUE;
}

void setup_subwindow(EuteranMainObject *wd, GtkButton *button)
{
    EutSubwindow *sub_window = euteran_main_object_get_subwindow(wd);

    if(eut_subwindow_get_opened_window(sub_window)){
        eut_subwindow_set_window_size(sub_window);
        eut_subwindow_set_window_visibility(sub_window, TRUE);
        return;
    }
    eut_subwindow_set_window_size(sub_window);

    GtkWindow *window = eut_subwindow_get_window(sub_window);
    GtkDropDown *theme_dropdown = eut_subwindow_get_dropdown(sub_window, DROPDOWN_THEME);
    GtkDropDown *icon_dropdown = eut_subwindow_get_dropdown(sub_window, DROPDOWN_ICON);

    eut_subwindow_init_audio_devices(sub_window);
    eut_subwindow_fill_hashtable(sub_window, ICON_HASH);
    eut_subwindow_fill_hashtable(sub_window, THEME_HASH);

    g_signal_connect(window, "close-request", G_CALLBACK(on_window_hide_request), sub_window);

    eut_subwindow_set_opened_window(sub_window, TRUE);
    eut_subwindow_set_window_visibility(sub_window, TRUE);
}
