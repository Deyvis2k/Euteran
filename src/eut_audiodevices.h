#pragma once

#include <gtk/gtk.h>

struct audio_device{
    char* node_description;
    char* node_name;
    int id;
};

struct audio_devices{
    GList* audio_device_sink;
    GList* audio_device_source;
};


GList* get_audio_devices(const char* command);
GList* get_command_output(const char* command);

