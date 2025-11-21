#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

/* Logging levels */
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

/* Initialize logging system */
void log_init(const char *filename);

/* Close logging system */
void log_cleanup(void);

/* Log a message with specified level */
void log_message(LogLevel level, const char *format, ...);

/* Convenience macros */
#define LOG_DEBUG(fmt, ...) log_message(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_message(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_message(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_message(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#endif
