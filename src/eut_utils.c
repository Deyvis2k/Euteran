#include "eut_utils.h"
#include "eut_constants.h"
#include "eut_async.h"
#include "eut_logs.h"
#include "eut_audiolinux.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

double* seconds_to_minute(double music_duration) {
    int min = music_duration * MINUTE_CONVERT;
    double seconds = fmod(music_duration, 60);
    

    double *result = (double*) malloc(2 * sizeof(double));
    if(result == NULL){
        log_error("Error allocating memory");
        exit(1);
    }

    result[0] = min;
    result[1] = seconds;
    return result;
}

char *cast_simple_double_to_string(double value) {
    if (value < 0){
        log_error("Value cannot be negative");
        return NULL;
    }

    char* buffer = malloc(10);  
    if (!buffer){
        log_error("Error allocating memory");
        return NULL;
    }

    snprintf(buffer, 10, "%.0f", value);
    return buffer;
}


char* cast_double_to_string(double value) {
    if (value < 0){
        log_error("Value cannot be negative");
        return NULL;
    }

    char* buffer = malloc(10);  
    if (!buffer){
        log_error("Error allocating memory");
        return NULL;
    }

    if (value > 60) {
        double* time_values = seconds_to_minute(value);
        if (!time_values) {
            free(buffer);
            log_error("Cannot get time values");
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
            free(time_values);
            return buffer;
        }
        if(s < 10){
            snprintf(buffer, 10, "%.0fm0%.0fs", m, s);
        }
        else{
            snprintf(buffer, 10, "%.0fm%.0fs", m, s);
        }
        free(time_values);
        return buffer;
    }

    snprintf(buffer, 10, "%.2fs", value);
    return buffer;
}

double string_to_double(const char *str){
    double value = atof(str);
    return value;
}



GList *list_files_musics(const char* dir) {
    DIR *dp;
    struct dirent *ep;
    GList *list = NULL;

    dp = opendir(dir);
    if (dp == NULL) {
        return list;
    }

    while ((ep = readdir(dp)) != NULL) {
        if (ep->d_type == DT_DIR || ep->d_name[0] == '.') {
            continue;
        }
        if (IS_ALLOWED_EXTENSION(ep->d_name)) {
            music_t *music = malloc(sizeof(music_t));
            if (music == NULL) {
                closedir(dp);
                return list;
            }

            music->name = strdup(ep->d_name);
            if (music->name == NULL) {
                free(music);
                closedir(dp);
                return list;
            }

            char* full_path = malloc(strlen(SYM_AUDIO_DIR) + strlen(ep->d_name) + 2);
            sprintf(full_path, "%s%s", SYM_AUDIO_DIR, ep->d_name);

            music->duration = get_duration(full_path);

            free(full_path);

            if (music->duration <= 0.0) {
                free(music->name);
                free(music);
                continue;
            }

            list = g_list_append(list, music);
            continue;
            
        }
    }

    closedir(dp);

    return list;
}

GFile* get_file_from_path() {
    GFile *css_file;
    if(g_file_test("Style/style.css", G_FILE_TEST_EXISTS)) {
        css_file = g_file_new_for_path("Style/style.css");
    } else {
        mkdir("Style", 0777);
        run_subprocess_async("touch Style/style.css", NULL, NULL);
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
        if(isdigit((unsigned char)*read) || *read == '.'){
            *write++ = *read;
        }
        read++;
    }
    *write = '\0';
}

void trim(char *str) {
    if (!str) return;
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    if (start != str) memmove(str, start, end - start + 2);
    if (!g_utf8_validate(str, -1, NULL)) {
        log_warning("String após trim inválida em UTF-8: %s", str);
    }
}

char* get_within_quotes(const char* str) {
    const char* start = strchr(str, '"');
    if (!start) {
        return NULL;
    }
    start++;

    const char* end = strchr(start, '"');
    if (!end) {
        return NULL;
    }

    size_t length = end - start;
    char* result = malloc(length + 1);
    if (!result) {
        log_error("Falha ao alocar memória");
        return NULL;
    }

    strncpy(result, start, length);
    result[length] = '\0';

    if (!g_utf8_validate(result, -1, NULL)) {
        log_warning("String extraída inválida em UTF-8: %s", result);
        gchar* converted = g_convert(result, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
        free(result);
        if (converted) {
            log_info("String convertida para UTF-8: %s", converted);
            return converted;
        } else {
            log_warning("Falha ao converter string para UTF-8");
            return g_strdup("[string inválida]");
        }
    }

    return result;
}


double
formatted_string_to_double(const char *str)
{
    if(strchr(str, 'm') == NULL && strchr(str, 'h') == NULL){
        log_warning("String inválida: %s", str);
        char *end_ptr;
        double value = strtod(str, &end_ptr);
        return value;
    }
    


    char temp[10] = {0};
    double value = 0;
    double multiplier = 1;
    for (const char *c = str; *c != '\0'; c++) {
        if(!isdigit((unsigned char)*c)){
            double val = strtod(temp, NULL);
            if(strcmp(str, "1m40s") == 0){
                log_info("temp: %s", temp);
            }
            if(*c == 'm'){
                multiplier = 60;
                val *= multiplier;
            }
            else if(*c == 'h'){
                multiplier = 3600;
                val *= multiplier;
            }

            temp[0] = '\0';
            value += val;
        }
        else{
            strcat(temp, c);
        }
    }
    
    return value;
}
