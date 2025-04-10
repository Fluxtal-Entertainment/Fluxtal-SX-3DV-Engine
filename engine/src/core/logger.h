#pragma once
#include "defines.h"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

//Disabling debug and trace for release build.
#if FRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
} log_level;

b8 initialize_logging();
void shutdown_logging();

FAPI void log_output(log_level level, const char* message, ...);

//Logs a fatal-level message.
#define FFATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);

#ifndef FERROR
//Logs an error-level message.
#define FERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
//Logs a warning-level message.
#define FWARN(message, ...) log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
//Does nothing when LOG_LEVEL_WARN != 1
#define FWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
//Logs a info-level message.
#define FINFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
//Does nothing when LOG_LEVEL_INFO != 1
#define FINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
//Logs a debug-level message.
#define FDEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
//Does nothing when LOG_LEVEL_DEBUG != 1
#define FDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
//Logs a trace-level message.
#define FTRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
//Does nothing when LOG_LEVEL_TRACE != 1
#define FTRACE(message, ...)
#endif