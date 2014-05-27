/*
 * log.h
 *
 *  Created on: 2012-5-9
 *      Author: buf1024@gmail.com
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>

#define LOG_DEBUG(...)                                               \
do{                                                                  \
    log_debug(__VA_ARGS__);                                          \
}while(0)                                                            \

#define LOG_INFO(...)                                                \
do{                                                                  \
    log_info(__VA_ARGS__);                                           \
}while(0)                                                            \

#define LOG_WARN(...)                                                \
do{                                                                  \
    log_warn(__VA_ARGS__);                                           \
}while(0)                                                            \

#define LOG_ERROR(...)                                               \
do{                                                                  \
    log_error(__VA_ARGS__);                                          \
}while(0)                                                            \

#define LOG_FATAL(...)                                               \
do{                                                                  \
    log_fatal(__VA_ARGS__);                                          \
}while(0)                                                            \



#define LOG_REG_CALLBACK(lvl, init_fun, init_args,                   \
        log_fun, log_args, uninit_fun, uninit_args)                  \
        log_reg_callback(lvl, init_fun, init_args,                   \
                log_fun, log_args, uninit_fun, uninit_args)

#define LOG_INITIALIZE() log_initialize()
#define LOG_INITIALIZE_DEFAULT(con_lvl, file_lvl, file_path,         \
            file_prefix, buf_size, sw_time, sw_size)                 \
        log_initialize_default(con_lvl, file_lvl, file_path,         \
                file_prefix, buf_size, sw_time, sw_size)

#define LOG_FINISH() log_finish()

#define LOG_FAIL                             -1
#define LOG_SUCCESS                           0

enum LogLevel
{
    LOG_ALL     = 0,
    LOG_DEBUG   = 100,
    LOG_INFO    = 200,
    LOG_WARN    = 300,
    LOG_ERROR   = 400,
    LOG_FATAL   = 500,
    LOG_OFF     = 600
};


typedef int (*log_callback_int_t)(void* args);
typedef int (*log_callback_log_t)(int loglvl, int reqlvl,
        const char* format,
        va_list ap, void* args);
typedef int (*log_callback_uninit_t)(void* args);

/* log callback function
 * lvl           trace level
 * init_cb       init function
 * init_args     init function arguments
 * log_cb        log function
 * log_args      log function arguments
 * uninit_cb     uninit function
 * uninit_args   uninit function arguments
 */
int log_reg_callback(int lvl,
        log_callback_int_t init_cb, void* init_args,
        log_callback_log_t log_cb, void* log_args,
        log_callback_uninit_t uninit_cb, void* uninit_args);


int log_initilize();
/* log_initialize_default
 * con_lvl     console appender trace log level
 * file_lvl    file appender trace log level
 * file_path   file appender log path
 * prefix      file appender name prefix
 * sw_time     file appender log switch time(in seconds)
 * sw_size     file appender log switch size(in bytes)
 */
int log_initialize_default(int con_lvl, int file_lvl,
        char* file_path, char* prefix, int buf_size,
        int sw_time, int sw_size);

int log_start();
int log_finish();

void log_clearup();

void log_debug(const char* format, ...);
void log_info (const char* format, ...);
void log_warn (const char* format, ...);
void log_error(const char* format, ...);
void log_fatal(const char* format, ...);

int log_get_level(const char* lvl);

#endif /* __LOG_H__ */
