#pragma once

#include <gtk/gtk.h>
#include <glib.h>

typedef struct{
    const gchar *music_path;
    const gchar *music_duration;
    const gchar *music_duration_raw;
} MusicDataInformation;

typedef struct {
    gchar *name;
    GList *music_list;
} ParsedBinaryData;


G_BEGIN_DECLS

#define TYPE_EUT_BINARY_PARSER (eut_binary_parser_get_type())

G_DECLARE_FINAL_TYPE(EutBinaryParser, eut_binary_parser, EUT, BINARY_PARSER, GObject)

EutBinaryParser *eut_binary_parser_new(void);

void 
eut_binary_parser_load_and_apply_binary(EutBinaryParser *self, void *main_object);

void
eut_binary_parser_save_binary(EutBinaryParser *self, void *main_object);




G_END_DECLS
