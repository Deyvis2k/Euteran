#include "euteran_settings.h"
#include "constants.h"
#include "e_logs.h"

struct _EuteranSettings {
    GObject parent_instance;

    int last_width;
    int last_height;
    float last_volume;
};

G_DEFINE_TYPE(EuteranSettings, euteran_settings, G_TYPE_OBJECT);


EuteranSettings *euteran_settings_new(void){
    return g_object_new(EUTERAN_TYPE_SETTINGS, NULL);
}

static void euteran_settings_init(EuteranSettings *self) {
    if(!g_file_test(CONFIGURATION_DIR, G_FILE_TEST_EXISTS)){
        g_mkdir_with_parents(CONFIGURATION_DIR, 0777);
    }

    self->last_height = 0;
    self->last_width = 0;
    self->last_volume = 0.500f;

    gchar *link_to_save = g_strdup_printf("%s/%s", CONFIGURATION_DIR, "current_settings.conf");

    FILE *file_to_read = fopen(link_to_save, "r");
    if(file_to_read == NULL){
        log_error("Erro ao abrir o arquivo para leitura");
        g_free(link_to_save);
        return;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), file_to_read)) {
        if (strncmp(line, "last_volume = ", 14) == 0) {
            float volume = atof(line + 14);
            self->last_volume = volume;
        } else if (strncmp(line, "last_width = ", 13) == 0) {
            int width = atoi(line + 13);
            self->last_width = width;
        } else if (strncmp(line, "last_height = ", 14) == 0) {
            int height = atoi(line + 14);
            self->last_height = height;
        }
    }
    
    fclose(file_to_read);
    g_free(link_to_save);
}

EuteranSettings *euteran_settings_get(void){
    static EuteranSettings *singleton = NULL;
    if (singleton == NULL) {
        singleton = euteran_settings_new();
    }
    return singleton;
}


static void euteran_settings_dispose(GObject *object) {
    G_OBJECT_CLASS(euteran_settings_parent_class)->dispose(object);
}

static void euteran_settings_class_init(EuteranSettingsClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = euteran_settings_dispose;
}

void euteran_settings_save(EuteranSettings *self, GtkWindow *window){
     g_return_if_fail(
        EUTERAN_IS_SETTINGS(self) &&
        GTK_IS_WINDOW(window)
    );
        

    if(!g_file_test(CONFIGURATION_DIR, G_FILE_TEST_EXISTS)){
        g_mkdir_with_parents(CONFIGURATION_DIR, 0777);
    }
    gchar *link_to_save = g_strdup_printf("%s/%s", CONFIGURATION_DIR, "current_settings.conf");
    FILE *file_to_save = fopen(link_to_save, "w");

    if(file_to_save == NULL){
        g_print("Erro ao abrir o arquivo para escrita\n");
        g_free(link_to_save);
        return;
    }

    int width, height;
    gtk_window_get_default_size(window, &width, &height);
    self->last_width = width;
    self->last_height = height;

    fprintf(file_to_save, "last_volume = %f\n", self->last_volume);
    fprintf(file_to_save, "last_width = %d\n", self->last_width);
    fprintf(file_to_save, "last_height = %d\n", self->last_height);

    fclose(file_to_save);
    g_free(link_to_save);
}

int euteran_settings_get_window_width(EuteranSettings *self){
    if(!EUTERAN_IS_SETTINGS(self)){
        log_error("EuteranSettings não é uma instância válida.");
        return 0;
    }

    return self->last_width;
}

int euteran_settings_get_window_height(EuteranSettings *self){
    if(!EUTERAN_IS_SETTINGS(self)){
        log_error("EuteranSettings não é uma instância válida.");
        return 0;
    }
    return self->last_height;
}

float euteran_settings_get_last_volume(EuteranSettings *self){
    if(!EUTERAN_IS_SETTINGS(self)){
        log_error("EuteranSettings não é uma instância válida.");
        return 0;
    }
    return self->last_volume;
}

void euteran_settings_set_last_volume(EuteranSettings *self, float volume){
    if(!EUTERAN_IS_SETTINGS(self)){
        log_error("EuteranSettings não é uma instância válida.");
        return;
    }
    self->last_volume = volume;
}
