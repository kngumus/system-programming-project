
#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static FILE *log_file = NULL;

int logger_init(const char *filename)
{
    if (filename == NULL) {
        return -1;
    }

    log_file = fopen(filename, "a");

    if (log_file == NULL) {
        perror("Failed to open log file");
        return -1;
    }

    return 0;
}

void logger_log(LogLevel level, const char *format, ...)
{
    if (log_file == NULL || format == NULL) {
        return;
    }

    time_t now = time(NULL);
    struct tm local_time;

    if (localtime_r(&now, &local_time) == NULL) {
        return;
    }

    char timestamp[32];

    if (strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", &local_time) == 0) {
        return;
    }

    const char *level_text;

    switch (level) {
        case LOG_INFO:
            level_text = "INFO";
            break;
        case LOG_WARNING:
            level_text = "WARNING";
            break;
        case LOG_ERROR:
            level_text = "ERROR";
            break;
        default:
            level_text = "UNKNOWN";
            break;
    }

    fprintf(log_file, "[%s] [%s] ", timestamp, level_text);

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file);
}

void logger_close(void)
{
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}
