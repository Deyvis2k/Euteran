#include "audio_devices.h"

char** get_command_output(const char* command){
    FILE *pipe = popen(command, "r");
    if(pipe == NULL){
        return NULL;
    }

    size_t max_size = 1024;
    char **output = malloc(max_size * sizeof(char*));
    if(!output){
        pclose(pipe);
        return NULL;
    }

    size_t current_size = 0;
    char buffer[max_size];

    while(fgets(buffer, max_size, pipe) != NULL){
        output[current_size] = malloc(strlen(buffer) + 1);
        if(!output[current_size]){
            for(size_t i = 0; i < current_size; i++){
                free(output[i]);
            }
            free(output);
            pclose(pipe);
            return NULL;
        }
        strcpy(output[current_size], buffer);
        if(isspace((unsigned char)buffer[strlen(buffer) - 1]))
            output[current_size][strlen(buffer) - 1] = '\0';
        current_size++;
        if(current_size == max_size){
            max_size *= 2;
            char **temp = realloc(output, max_size * sizeof(char*));
            if(!temp){
                for(size_t i = 0; i < current_size; i++){
                    free(output[i]);
                }
                free(output);
                pclose(pipe);
                return NULL;
            }
            output = temp;
        }
    }
    output[current_size] = NULL;
    pclose(pipe);
    return output;
}



struct audio_device** get_audio_devices(const char* command) {
    char** output = get_command_output(command);
    if (!output) {
        printf("Erro: get_command_output retornou NULL\n");
        return NULL;
    }

    size_t max_items = 10;
    struct audio_device** sinks = malloc(max_items * sizeof(struct audio_device*));
    if (!sinks) {
        printf("Erro: falha ao alocar sinks\n");
        for (size_t i = 0; output[i] != NULL; i++) free(output[i]);
        free(output);
        return NULL;
    }

    size_t count = 0;
    char* temp_description = NULL;
    char* temp_name = NULL;
    int temp_id = -1;

    for (size_t i = 0; output[i] != NULL; i++) {
        trim(output[i]);
        if (strstr(output[i], "node.description") != NULL) {
            free(temp_description);
            temp_description = get_within_quotes(strrchr(output[i], ':'));
            if (temp_description) {
                char* temp = strdup(temp_description);
                temp_description = temp;
            }
        }
        else if (strstr(output[i], "node.name") != NULL) {
            free(temp_name);
            temp_name = get_within_quotes(strrchr(output[i], ':'));
            if (temp_name) {
                char* temp = strdup(temp_name);
                temp_name = temp;
            }
        }
        else if (strstr(output[i], "object.id") != NULL) {
            char* colon = strrchr(output[i], ':');
            if (colon) {
                colon++;
                while (isspace((unsigned char)*colon)) colon++; 
                temp_id = atoi(colon); 

                if (temp_id != -1 && (temp_description || temp_name)) {
                    if (count == max_items) {
                        max_items *= 2;
                        struct audio_device** temp_sinks = realloc(sinks, max_items * sizeof(struct audio_device*));
                        if (!temp_sinks) {
                            for (size_t j = 0; j < count; j++) {
                                free(sinks[j]->node_name);
                                free(sinks[j]->node_description);
                                free(sinks[j]);
                            }
                            free(sinks);
                            free(temp_description);
                            free(temp_name);
                            for (size_t j = 0; output[j] != NULL; j++) free(output[j]);
                            free(output);
                            return NULL;
                        }
                        sinks = temp_sinks;
                    }

                    struct audio_device* new_sink = malloc(sizeof(struct audio_device));
                    if (!new_sink) {
                        return NULL;
                    }

                    new_sink->id = temp_id;
                    new_sink->node_description = temp_description ? strdup(temp_description) : NULL;
                    new_sink->node_name = temp_name ? strdup(temp_name) : NULL;
                    sinks[count++] = new_sink;

                    temp_description = NULL;
                    temp_name = NULL;
                    temp_id = -1;
                }
            }
        }
    }

    if (temp_id != -1 && (temp_description || temp_name)) {
        if (count == max_items) {
            max_items += 1;
            struct audio_device** temp_sinks = realloc(sinks, max_items * sizeof(struct audio_device*));
            if (!temp_sinks) {
                return NULL;
            }
            sinks = temp_sinks;
        }

        struct audio_device* new_sink = malloc(sizeof(struct audio_device));
        if (!new_sink) {
            return NULL;
        }

        new_sink->id = temp_id;
        new_sink->node_description = temp_description ? strdup(temp_description) : NULL;
        new_sink->node_name = temp_name ? strdup(temp_name) : NULL;
        sinks[count++] = new_sink;

    } else {
        free(temp_description);
        free(temp_name);
    }

    sinks[count] = NULL;

    for (size_t i = 0; output[i] != NULL; i++) free(output[i]);
    free(output);

    return sinks;
}
