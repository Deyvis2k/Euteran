#pragma once

#define COMMAND_AUDIO_SOURCE "pw-dump | grep -A 50 'Audio/Source' | grep -E 'object.id|node.name|node.description'"
#define COMMAND_AUDIO_SINK "pw-dump | grep -A 50 'Audio/Sink' | grep -E 'object.id|node.name|node.description'"

#define COMMAND_VIRTUAL_AUDIO_SINK "pw-dump | grep -A 10 'VirtualMicSink' | grep -E 'object.id|node.name|node.description'"
#define COMMAND_VIRTUAL_AUDIO_SOURCE "pw-dump | grep -A 10 'VirtualMicSource' | grep -E 'object.id|node.name|node.description'"

#include <gtk/gtk.h>

void construct_widget(GtkWidget *button, gpointer user_data);
void construct_virtual_audio_widget(GtkWidget *grid);
gboolean on_key_press(GtkEventControllerKey *controller, guint keyval,
    guint keycode,
    GdkModifierType state,
    gpointer user_data);


