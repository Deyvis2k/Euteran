#include "eut_dialogs.h"
#include "adwaita.h"
#include "eut_logs.h"
#include "glib/gstdio.h"

#define _(str) (str)

static void 
on_remove_music(
    gpointer    user_data
) 
{
    EuteranDialogContainer *container = (EuteranDialogContainer *)user_data;

    if(!container->path){
        log_info("Path not found at line %d", __LINE__);
    }

    if(g_unlink(container->path) != 0){
        log_error("Erro ao remover musica: %s", g_strerror(errno));
        return;
    }

    gtk_list_box_remove(GTK_LIST_BOX(container->list_box), container->row);
}


static void
dialog_cb(
    AdwAlertDialog   *dialog,       
    GAsyncResult     *result,
    gpointer         user_data
)
{
    g_return_if_fail (dialog != NULL && result != NULL && user_data != NULL);

    EuteranDialogContainer *container = (EuteranDialogContainer *)  user_data;
    
    if(!container->path){
        log_info("Path not found at line %d", __LINE__);
    }

    const char *response = adw_alert_dialog_choose_finish (dialog, result);
    if (g_strcmp0 (response, "remove") == 0) {
        on_remove_music(container);
    }
}



static void 
create_remove_music_button(
    const char                 *music_name,
    EuteranDialogContainer     *container
){
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


    adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "cancel");
    adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");

    adw_alert_dialog_choose (ADW_ALERT_DIALOG (dialog), GTK_WIDGET (container->window_parent),
                           NULL, (GAsyncReadyCallback) dialog_cb, container);

}

void on_pressed_right_click_event(
    GtkGestureClick     *gesture,
    int                 n_press,
    double              x,
    double              y,
    gpointer            user_data
)
{

    EuteranDialogContainer *container = (EuteranDialogContainer *)user_data;

    if (gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture)) == GDK_BUTTON_SECONDARY) {

        GtkWidget *label_music_name = gtk_widget_get_first_child(container->row_box);

        if (!GTK_IS_LABEL(label_music_name)) {
            log_error("O primeiro widget do GtkBox nao eh um GtkLabel");
            return;
        }

        const char *music_name = gtk_label_get_text(GTK_LABEL(label_music_name));
        
        create_remove_music_button(music_name, container);
    }   
}
