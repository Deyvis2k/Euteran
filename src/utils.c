#include "utils.h"
#include "audio.h"
#include "constants.h"
#include "gio/gio.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "constants.h"

#define ALLOWED_EXTENSION ".mp3"


const double* seconds_to_minute(double music_duration) {
    int min = music_duration * MINUTE_CONVERT;
    double seconds = fmod(music_duration, 60);
    

    //appends to array
    double *result = (double*) malloc(2 * sizeof(double));
    if(result == NULL){
        fprintf(stderr, "Error allocating memory\n");
        exit(1);
    }

    result[0] = min;
    result[1] = seconds;
    return result;
}

double get_duration_from_file(const char *file) {
    double duration = get_duration(file);
    return duration;
}


char* cast_double_to_string(double value) {
    if (value < 0){
        fprintf(stderr, "Error allocating memory in value\n");
        return NULL;
    }

    char* buffer = malloc(10);  
    if (!buffer){
        fprintf(stderr, "Error allocating memory\n");
        return NULL;
    }

    if (value > 60) {
        const double* time_values = seconds_to_minute(value);
        if (!time_values) {
            free(buffer);
            fprintf(stderr, "Error allocating memory in time_values\n");
            return NULL;
        }
        double m = time_values[0];
        double s = time_values[1];

        snprintf(buffer, 10, "%.0fm%.0fs", m, s);
        return buffer;
    }

    snprintf(buffer, 10, "%.2fs", value);
    return buffer;
}



music_list_t list_files_musics(const char* dir) {
    DIR *dp;
    struct dirent *ep;
    music_t *musics = NULL;  
    size_t total = 0; 
    music_list_t list = {NULL, 0};
    size_t count = 0;

    dp = opendir(dir);
    if (dp == NULL) {
        return list;
    }

    while ((ep = readdir(dp)) != NULL) {
        if (ep->d_type == DT_DIR || ep->d_name[0] == '.') {
            continue;
        }

        if (strstr(ep->d_name, ALLOWED_EXTENSION)) {
            // Aloca espaço para mais um elemento
            music_t* temp = realloc(musics, sizeof(music_t) * (total + 1));
            if (temp == NULL) {
                if(musics){
                    for(size_t i = 0; i < total; i++){
                        free(musics[i].name);
                    }
                    free(musics);
                }
                closedir(dp);
                return list;
            }
            musics = temp;

            // Copia o nome do arquivo para evitar sobrescrita
            musics[total].name = strdup(ep->d_name);
            if (musics[total].name == NULL) {
                for(size_t i = 0; i < total; i++){
                    free(musics[i].name);
                }
                free(musics);
                closedir(dp);
                return list;
            }
            // Copia o caminho completo do arquivo
            char* full_path = malloc(strlen(SYM_AUDIO_DIR) + strlen(ep->d_name) + 2);
            sprintf(full_path, "%s%s", SYM_AUDIO_DIR, ep->d_name);
            musics[total].duration = get_duration(full_path);
            free(full_path);
            total++;
        }
    }

    closedir(dp);

    count = total;

    list.musics = musics;
    list.count_size = count;
    
    


    return list;
}

GFile* get_file_from_path() {
    GFile *css_file;
    if(g_file_test("/home/deyvis/Documents/Soundpadlinux/Style/style.css", G_FILE_TEST_EXISTS)) {
        css_file = g_file_new_for_path("/home/deyvis/Documents/Soundpadlinux/Style/style.css");
    } else {
        mkdir("/home/deyvis/Documents/Soundpadlinux/Style", 0777);
        system("touch /home/deyvis/Documents/Soundpadlinux/Style/style.css");
        css_file = g_file_new_for_path("/home/deyvis/Documents/Soundpadlinux/Style/style.css");
    }

    return css_file;
}
