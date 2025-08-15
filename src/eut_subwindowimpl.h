#pragma once

#include "eut_subwindow.h"
#include "glib.h"
#include <gtk/gtk.h>
#include "eut_main_object.h"

void setup_subwindow(EuteranMainObject *wd, GtkButton *button);
gboolean on_key_press(GtkEventControllerKey *controller, guint keyval,
    guint keycode,
    GdkModifierType state,
    gpointer user_data);


gboolean
on_dropdown_theme_selected
(
    EutSubwindow *sub_window,
    GParamSpec *pspec,
    GObject    *obj
);

gboolean
on_dropdown_icon_selected
(
    EutSubwindow *sub_window,
    GParamSpec *pspec,
    GObject    *obj
);
