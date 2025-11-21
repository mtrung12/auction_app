// src/common/utils.c
#include "utils.h"
#include <stdlib.h>
#include <pthread.h>

static FILE *log_file = NULL;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

static const char* level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

void log_init(const char *filename) {
    pthread_mutex_lock(&log_lock);
    if (log_file) {
        fclose(log_file);
    }
    log_file = fopen(filename, "a");
    pthread_mutex_unlock(&log_lock);
}

void log_cleanup(void) {
    pthread_mutex_lock(&log_lock);
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_lock);
}

void log_message(LogLevel level, const char *format, ...) {
    pthread_mutex_lock(&log_lock);
    
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    FILE *output = log_file ? log_file : stderr;
    fprintf(output, "[%s] [%s] ", timestamp, level_to_string(level));
    
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);
    
    fprintf(output, "\n");
    fflush(output);
    
    pthread_mutex_unlock(&log_lock);
}
