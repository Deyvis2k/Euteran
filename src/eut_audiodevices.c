#include "eut_audiodevices.h"
#include "eut_utils.h"
#include <ctype.h>
#include "eut_logs.h"

GList* get_command_output(const char* command){
    FILE *pipe = popen(command, "r");
    if(pipe == NULL){
        perror("popen");
        return NULL;
    }
    GList *list = NULL;

    char buffer[1024];
    while(fgets(buffer, sizeof(buffer), pipe) != NULL){
        if (isspace((unsigned char)buffer[strlen(buffer) - 1]))
            buffer[strlen(buffer) - 1] = '\0';
        list = g_list_append(list, g_strdup(buffer));
    }

    pclose(pipe);
    return list;
}

GList* get_audio_devices(const char* command) {
    GList* output = get_command_output(command);
    if (!output) {
        printf("Erro: get_command_output retornou NULL\n");
        return NULL;
    }

    GList *desired_list = NULL;
    char *node_name = NULL;
    char *node_description = NULL;
    int id = -1;

    for (GList *node = output; node != NULL; node = node->next) {
        trim((char*)node->data);
        if (strstr(node->data, "node.description") != NULL) {
            char *description_raw = get_within_quotes(strrchr((char*)node->data, ':'));
            if (description_raw) {
                if (g_utf8_validate(description_raw, -1, NULL)) {
                    node_description = g_strdup(description_raw);
                } else {
                    gchar *converted = g_locale_to_utf8(description_raw, -1, NULL, NULL, NULL);
                    if (converted || g_utf8_validate(converted, -1, NULL)) {
                        node_description = converted;
                    } else {
                        log_warning("Falha ao converter descrição para UTF-8");
                    }
                }
                free(description_raw);
            }
        } else if (strstr(node->data, "node.name") != NULL) {
            char *name_raw = get_within_quotes(strrchr((char*)node->data, ':'));
            if (name_raw) {
                if (g_utf8_validate(name_raw, -1, NULL)) {
                    node_name = g_strdup(name_raw);
                } else {
                    gchar *converted = g_locale_to_utf8(name_raw, -1, NULL, NULL, NULL);
                    if (converted || g_utf8_validate(converted, -1, NULL)) {
                        node_name = converted;
                    } else {
                        log_warning("Falha ao converter nome para UTF-8");
                    }
                }
                free(name_raw);
            }

        } else if (strstr(node->data, "object.id") != NULL) {
            char* colon = strrchr((char*)node->data, ':');
            if (colon) {
                colon++;
                while (isspace((unsigned char)*colon)) colon++;
                id = atoi(colon);
            }
        }
        if(id != -1 && node_name && node_description){
            struct audio_device *audio_device = malloc(sizeof(struct audio_device));
            audio_device->id = id;
            audio_device->node_description = node_description ? g_strdup(node_description) : NULL;
            audio_device->node_name = node_name ? g_strdup(node_name) : NULL;       
            desired_list = g_list_append(desired_list, audio_device);
            id = -1;
            g_free(node_name);
            g_free(node_description);
            node_name = NULL;
            node_description = NULL;
        }
    }
    g_list_free(output);
    return desired_list;
}
