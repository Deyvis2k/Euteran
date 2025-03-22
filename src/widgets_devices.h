#ifndef WIDGETS_DEVICES_H
#define WIDGETS_DEVICES_H


#define COMMAND_AUDIO_SOURCE "pw-dump | grep -A 50 'Audio/Source' | grep -E 'object.id|node.name|node.description'"
#define COMMAND_AUDIO_SINK "pw-dump | grep -A 50 'Audio/Sink' | grep -E 'object.id|node.name|node.description'"

#include <gtk/gtk.h>
#include "audio_devices.h"

void construct_widget(GtkWidget *button);

#endif
