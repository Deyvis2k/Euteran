#pragma once

#include <stdarg.h>

typedef enum {
    LOG_MSG,
    LOG_ERR,
    LOG_INFO,
    LOG_WARN,
    LOG_CMD,
    LOG_COUNT          
} log_level_t;

extern const char *level_tag[LOG_COUNT];
extern const char *level_color[LOG_COUNT];

void vlog_base(
    log_level_t lvl, 
    const char *file,
    int line,
    const char *func,
    const char *format, 
    va_list ap
) __attribute__((format(printf,5,0)));

void log_base(
    log_level_t lvl,
    const char *file,
    int line,
    const char *func,
    const char *format, 
    ...) __attribute__((format(printf,5,6)));



#define log_message(...) log_base(LOG_MSG,__FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_error(...) log_base(LOG_ERR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_info(...) log_base(LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_warning(...) log_base(LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_command(...) log_base(LOG_CMD, __FILE__, __LINE__, __func__, __VA_ARGS__)
