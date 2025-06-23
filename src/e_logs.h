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

void vlog_base(log_level_t lvl, 
                      const char *format, va_list ap) __attribute__((format(printf,2,0)));
void vlog_base(log_level_t lvl, const char *format, va_list ap);
void log_base(log_level_t lvl,
                     const char *format, ...) __attribute__((format(printf,2,3)));
void log_base(log_level_t lvl, const char *format, ...);



#define log_message(...) log_base(LOG_MSG, __VA_ARGS__)
#define log_error(...) log_base(LOG_ERR, __VA_ARGS__)
#define log_info(...) log_base(LOG_INFO, __VA_ARGS__)
#define log_warning(...) log_base(LOG_WARN, __VA_ARGS__)
#define log_command(...) log_base(LOG_CMD, __VA_ARGS__)
