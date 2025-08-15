#pragma once

#include <gtk/gtk.h>


typedef struct {
    GtkWidget *row_box;
    GtkWidget *row;
    GtkWidget *list_box;
    GtkWidget *window_parent;
    gchar     *path;
    void      *aditional_pointer;
}EuteranDialogContainer;



void on_pressed_right_click_event(
    GtkGestureClick     *gesture,
    int                 n_press,
    double              x,
    double              y,
    gpointer            user_data
);


void on_removed_music_event_window(
    GtkWidget   *widget,
    gpointer    user_data
);
