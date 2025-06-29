#pragma once

#include <stdio.h>

struct audio_device{
    char* node_description;
    char* node_name;
    int id;
};

struct audio_devices{
    struct audio_device **sink;
    struct audio_device **source;
    size_t sink_count;
    size_t source_count;
};


struct audio_device** get_audio_devices(const char* command);
char** get_command_output(const char* command);

