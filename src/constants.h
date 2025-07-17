#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    SYMLINK,
    CONFIGURATION
} LOCATION_T;

typedef enum{
    MP3,
    OGG,
    WAV,
    MAX_MUSIC_TYPES
} AUDIO_TYPE;

static char* get_sym_audio_dir(LOCATION_T location) {
    const char* title = location == SYMLINK ? "symlinks" : "configuration";

#ifdef __linux__
    const char* base = getenv("XDG_DATA_HOME");
    if (!base || base[0] == '\0') {
        base = getenv("HOME");
        if (!base) return NULL;

        size_t len = strlen(base) + strlen("/.local/share/Euteran/") + strlen(title) + 2;
        char* path = malloc(len);
        if (!path) return NULL;
        snprintf(path, len, "%s/.local/share/Euteran/%s/", base, title);
        return path;
    } else {
        size_t len = strlen(base) + strlen("/Euteran/") + strlen(title) + 2;
        char* path = malloc(len);
        if (!path) return NULL;
        snprintf(path, len, "%s/Euteran/%s/", base, title);
        return path;
    }

#elif _WIN32
    const char* user = getenv("USERNAME");
    if (!user) return NULL;

    // Choose correct subdir
    const char* subdir = location == SYMLINK ? "symlinks" : "configuration";

    size_t len = strlen("C:\\Users\\") + strlen(user) +
                 strlen("\\AppData\\Local\\Euteran\\") + strlen(subdir) + 2;
    char* path = malloc(len);
    if (!path) return NULL;

    snprintf(path, len, "C:\\Users\\%s\\AppData\\Local\\Euteran\\%s\\", user, subdir);
    return path;
#else
    return NULL;
#endif
}

#define SYM_AUDIO_DIR get_sym_audio_dir(SYMLINK)
#define CONFIGURATION_DIR get_sym_audio_dir(CONFIGURATION)



#define RED_COLOR  "\033[31m"           
#define GREEN_COLOR "\033[32m"          
#define YELLOW_COLOR  "\033[33m"        
#define BLUE_COLOR    "\033[34m"        
#define MAGENTA_COLOR    "\033[35m"     
#define CYAN_COLOR    "\033[36m"        
#define RESET_COLOR    "\033[0m"        

