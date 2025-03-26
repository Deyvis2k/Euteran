#include "utils.h"
#include "audio.h"
#include "constants.h"
#include "gio/gio.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "constants.h"

#define ALLOWED_EXTENSION ".mp3"


const double* seconds_to_minute(double music_duration) {
    int min = music_duration * MINUTE_CONVERT;
    double seconds = fmod(music_duration, 60);
    

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
        double h = 0;
        double m = time_values[0];
        if(m >= 60){
            h = m / 60;
            m = fmod(m, 60);
        }
        double s = time_values[1];
        if(h > 0){
            snprintf(buffer, 10, "%.0fh%.0fm%.0fs", h, m, s);
            return buffer;
        }
        if(s < 10){
            snprintf(buffer, 10, "%.0fm0%.0fs", m, s);
        }
        else{
            snprintf(buffer, 10, "%.0fm%.0fs", m, s);
        }
        return buffer;
    }

    snprintf(buffer, 10, "%.2fs", value);
    return buffer;
}

double string_to_double(const char *str){
    double value = atof(str);
    return value;
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
        //if is .mp3 or .ogg
        if (strstr(ep->d_name, ALLOWED_EXTENSION) != NULL || strstr(ep->d_name, ".ogg") != NULL) {
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

            musics[total].name = strdup(ep->d_name);
            if (musics[total].name == NULL) {
                for(size_t i = 0; i < total; i++){
                    free(musics[i].name);
                }
                free(musics);
                closedir(dp);
                return list;
            }
            char* full_path = malloc(strlen(SYM_AUDIO_DIR) + strlen(ep->d_name) + 2);
            sprintf(full_path, "%s%s", SYM_AUDIO_DIR, ep->d_name);

            if(strstr(ep->d_name, ".ogg") != NULL){
                musics[total].duration = get_duration_ogg(full_path);
            }else
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
    if(g_file_test("Style/style.css", G_FILE_TEST_EXISTS)) {
        css_file = g_file_new_for_path("Style/style.css");
    } else {
        mkdir("Style", 0777);
        system("touch Style/style.css");
        css_file = g_file_new_for_path("Style/style.css");
    }

    return css_file;
}


char* remove_outside_quotes(char* str){
    size_t len = strlen(str);
    if(len > 1 && str[0] == '"' && str[len - 1] == '"'){
        str[len - 1] = '\0';
        return str + 1;
    }
    return str;
}



void remove_if_not_number(char* str){
    char* read = str;
    char* write = str;
    while(*read){
        if(isdigit((unsigned char)*read)){
            *write++ = *read;
        }
        read++;
    }
    *write = '\0';
}

void trim(char* str) {
    char* end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

char* get_within_quotes(const char* str) {
    const char* start = strchr(str, '"');  
    if (!start) return NULL;
    start++;

    const char* end = strchr(start, '"');  
    if (!end) return NULL;

    size_t length = end - start;
    char* result = malloc(length + 1);
    if (!result) return NULL;

    strncpy(result, start, length);
    result[length] = '\0';

    return result;
}
