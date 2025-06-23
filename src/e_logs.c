#include "e_logs.h"
#include "constants.h"
#include <time.h>
#include <stdio.h>


const char *level_tag [LOG_COUNT] = {
    "LOG", "ERROR", "INFO", "WARNING", "COMMAND"
};

const char *level_color [LOG_COUNT] = {
    MAGENTA_COLOR, RED_COLOR, GREEN_COLOR, YELLOW_COLOR, BLUE_COLOR
};


void vlog_base(log_level_t lvl, const char *format, va_list ap)
{
    char tstamp[9];               
    time_t now = time(NULL);
    strftime(tstamp, sizeof tstamp, "%H:%M:%S", localtime(&now));

    FILE *out = stdout;

    fprintf(out, "%s%s [%s] ", level_color[lvl], tstamp, level_tag[lvl]);
    vfprintf(out, format, ap);
    fprintf(out, RESET_COLOR "\n");
}

void log_base(log_level_t lvl, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vlog_base(lvl, format, ap);
    va_end(ap);
}
