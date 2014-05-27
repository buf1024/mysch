/*
 * log.c
 *
 *  Created on: 2012-5-9
 *      Author: buf1024@gmail.com
 */

#include "log.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#define LOG_DEFAULT_HEAD_SIZE   128
#define LOG_DEFAULT_BUFFER_SIZE 4096
#define LOG_DEFAULT_MAX_PATH    256
//////////////////////////////////////////////////////////////////////
// built-in appender
// console/term appender
static int log_console_callback_init(void* args);
static int log_console_callback_log(int loglvl, int reqlvl, const char* format,
        va_list ap, void* args);
static int log_console_callback_uninit(void* args);
// file appender
static int log_file_init_callback_fun(void* args);
static int log_file_log_callback_fun(int loglvl, int reqlvl, const char* format,
        va_list ap, void* args);
static int log_file_uninit_callback_fun(void* args);

struct file_log
{
    char  path[LOG_DEFAULT_MAX_PATH];
    char  prefix[LOG_DEFAULT_MAX_PATH];
    FILE* fp;
    int   file_count;
    int   switch_time;
    int   switch_flag;
    int   switch_size;

    char* buf;
    int   buf_size;
    int   w_size;
    int   w_pos;
};


struct call_back
{
    int level;

    log_callback_int_t init_cb_fun;
    void* init_cb_args;

    log_callback_log_t log_cb_fun;
    void* log_cb_args;

    log_callback_uninit_t uninit_cb_fun;
    void* uninit_cb_args;

    struct call_back* next;
};

struct log_context
{
    struct call_back* cb;
};

static struct log_context _context   = {0};
static pthread_mutex_t     _ctx_mutex = PTHREAD_MUTEX_INITIALIZER;

struct file_log  _file_log_ctx;

static int log_get_header(int lvl, char* buf, int size)
{
    const char* head = 0;
    switch(lvl)
    {
    case LOG_DEBUG:
        head = "[D]";
        break;
    case LOG_INFO:
        head = "[I]";
        break;
    case LOG_WARN:
        head = "[W]";
        break;
    case LOG_ERROR:
        head = "[E]";
        break;
    case LOG_FATAL:
        head = "[F]";
        break;
    default:
        break;
    }
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);
    int ret = snprintf(buf, size, "%s[%4d%2d%2d%2d%2d%2d:%ld]", head,
            tm->tm_year,tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec, (long)tv.tv_usec);

    return ret;
}

static void log_message(int lvl, const char* format, va_list ap)
{
    struct call_back* cb = _context.cb;
    while(cb){
        if (cb->log_cb_fun) {
            if (cb->log_cb_fun(cb->level, lvl, format, ap,
                    cb->log_cb_args) == LOG_FAIL) {
                fprintf(stderr, "fail to log message!\n");
            }
        }
        cb = cb->next;
    }
}

int log_get_level(const char* lvl)
{
    if (lvl) {
        if (strcasecmp(lvl, "all") == 0) {
            return LOG_ALL;
        } else if (strcasecmp(lvl, "debug") == 0) {
            return LOG_DEBUG;
        } else if (strcasecmp(lvl, "info") == 0) {
            return LOG_INFO;
        } else if (strcasecmp(lvl, "warn") == 0) {
            return LOG_WARN;
        } else if (strcasecmp(lvl, "error") == 0) {
            return LOG_ERROR;
        } else if (strcasecmp(lvl, "fatal") == 0) {
            return LOG_FATAL;
        } else if (strcasecmp(lvl, "off") == 0) {
            return LOG_OFF;
        }
    }

    return LOG_ALL;
}

int log_reg_callback(int lvl,
        log_callback_int_t init_cb, void* init_args,
        log_callback_log_t log_cb, void* log_args,
        log_callback_uninit_t uninit_cb, void* uninit_args)
{
    pthread_mutex_lock(&_ctx_mutex);
    struct call_back* cb = (struct call_back*)malloc(sizeof(struct call_back));
    if(cb){
        cb->level = lvl;//log_get_level(lvl);
        cb->init_cb_fun = init_cb;
        cb->init_cb_args = init_args;
        cb->log_cb_fun = log_cb;
        cb->log_cb_args = log_args;
        cb->uninit_cb_fun = uninit_cb;
        cb->uninit_cb_args = uninit_args;
        cb->next = 0;

        if(_context.cb == 0){
            _context.cb = cb;
        }else{
            struct call_back* iter_cb = _context.cb;
            while(iter_cb->next){
                iter_cb = iter_cb->next;
            }
            iter_cb->next = cb;
        }
    }
    pthread_mutex_unlock(&_ctx_mutex);
    return LOG_SUCCESS;
}
int log_start()
{
    struct call_back* cb = _context.cb;
    while(cb){
        if (cb->init_cb_fun) {
            if (cb->init_cb_fun(cb->init_cb_args) == LOG_FAIL) {
                fprintf(stderr, "fail to init call back!\n");
                return LOG_FAIL;
            }
        }
        cb = cb->next;
    }
    return LOG_SUCCESS;
}

void log_clearup()
{
    memset(&_context, 0, sizeof(_context));
    memset(&_ctx_mutex, 0, sizeof(_ctx_mutex)); //PTHREAD_MUTEX_INITIALIZER;
}

void log_debug(const char* format, ...)
{
    va_list ap;

    pthread_mutex_lock(&_ctx_mutex);
    va_start(ap, format);
    log_message(LOG_DEBUG, format, ap);
    va_end(ap);
    pthread_mutex_unlock(&_ctx_mutex);
}
void log_info (const char* format, ...)
{
    va_list ap;

    pthread_mutex_lock(&_ctx_mutex);
    va_start(ap, format);
    log_message(LOG_INFO, format, ap);
    va_end(ap);
    pthread_mutex_unlock(&_ctx_mutex);
}
void log_warn (const char* format, ...)
{
    va_list ap;

    pthread_mutex_lock(&_ctx_mutex);
    va_start(ap, format);
    log_message(LOG_WARN, format, ap);
    va_end(ap);
    pthread_mutex_unlock(&_ctx_mutex);
}
void log_error(const char* format, ...)
{
    va_list ap;

    pthread_mutex_lock(&_ctx_mutex);
    va_start(ap, format);
    log_message(LOG_ERROR, format, ap);
    va_end(ap);
    pthread_mutex_unlock(&_ctx_mutex);
}
void log_fatal(const char* format, ...)
{
    va_list ap;

    pthread_mutex_lock(&_ctx_mutex);
    va_start(ap, format);
    log_message(LOG_FATAL, format, ap);
    va_end(ap);
    pthread_mutex_unlock(&_ctx_mutex);
}

int log_finish()
{
    pthread_mutex_lock(&_ctx_mutex);

    struct call_back* cb = _context.cb;
    while(cb){
        struct call_back* tmp_cb = cb;
        if (cb->uninit_cb_fun) {
            if (cb->uninit_cb_fun(cb->uninit_cb_args) == LOG_FAIL) {
                fprintf(stderr, "fail to uninit call back!\n");
            }
        }
        cb = cb->next;
        free(tmp_cb);
    }

    pthread_mutex_unlock(&_ctx_mutex);

    return LOG_SUCCESS;
}
int log_initialize_default(int con_lvl, int file_lvl,
        char* file_path, char* prefix, int buf_size,
        int sw_time, int sw_size)
{
    log_clearup();

    if(!file_path || !prefix || buf_size <= 0) {
        return LOG_FAIL;
    }

    int ret = log_reg_callback(con_lvl, log_console_callback_init, 0,
            log_console_callback_log, 0, log_console_callback_uninit,
            0);
    if (ret == LOG_FAIL) {
        return LOG_FAIL;
    }

    memset(&_file_log_ctx, 0, sizeof(_file_log_ctx));

    strncpy(_file_log_ctx.path, file_path, sizeof(_file_log_ctx.path) - 1);
    strncpy(_file_log_ctx.prefix, prefix, sizeof(_file_log_ctx.prefix) - 1);
    _file_log_ctx.buf_size = buf_size;
    _file_log_ctx.switch_size = sw_size;
    _file_log_ctx.switch_time = sw_time;
    if(_file_log_ctx.buf) {
        free(_file_log_ctx.buf);
    }
    _file_log_ctx.buf = malloc(sizeof(char)*buf_size);
    if(!_file_log_ctx.buf) {
        return LOG_FAIL;
    }

    ret = log_reg_callback(file_lvl, log_file_init_callback_fun,
            &_file_log_ctx, log_file_log_callback_fun, 0,
            log_file_uninit_callback_fun, 0);
    if (ret == LOG_FAIL) {
        return LOG_FAIL;
    }
    return log_start();
}
int log_initilize()
{
    log_clearup();
    return log_start();
}


/////////////////////////////////////
// built-in console appender
int log_console_callback_init(void* args)
{
    return LOG_SUCCESS;
}
int log_console_callback_log(int loglvl, int reqlvl, const char* format,
        va_list ap, void* args)
{
    if(reqlvl >= loglvl){
        char head[LOG_DEFAULT_HEAD_SIZE] = {0};
        log_get_header(reqlvl, head, LOG_DEFAULT_HEAD_SIZE);

        fprintf(stdout, head);
        vfprintf(stdout, format, ap);
    }
    return LOG_SUCCESS;
}
int log_console_callback_uninit(void* args)
{
    return LOG_SUCCESS;
}

//built-in file appender
int log_file_init_callback_fun(void* args)
{
    FILE* fp = _file_log_ctx.fp;
    if(fp != NULL){
        fclose(fp);
    }
    char file[LOG_DEFAULT_MAX_PATH] = {0};
    pid_t pid = getpid();

    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);

    snprintf(file, LOG_DEFAULT_MAX_PATH, "%s/%s_%d_%4d%2d%2d_%d.tmp",
            _file_log_ctx.path, _file_log_ctx.prefix, pid,
            tm->tm_year,tm->tm_mon + 1, tm->tm_mday, _file_log_ctx.file_count);

    fp = fopen(file, "a+");
    if(fp == NULL){
        return LOG_FAIL;
    }
    return LOG_SUCCESS;
}
int log_file_log_callback_fun(int loglvl, int reqlvl, const char* format,
        va_list ap, void* args)
{
    FILE* fp = _file_log_ctx.fp;
    if(fp == NULL){
        return LOG_FAIL;
    }
    if(reqlvl >= loglvl){


        char head[LOG_DEFAULT_HEAD_SIZE] = {0};
        int h_size = log_get_header(reqlvl, head, LOG_DEFAULT_HEAD_SIZE);

        char log[LOG_DEFAULT_BUFFER_SIZE] = {0};

        int m_size = vsnprintf(log, LOG_DEFAULT_BUFFER_SIZE, format, ap);

        if((m_size + h_size) > (_file_log_ctx.buf_size - _file_log_ctx.w_pos)) {
            if(_file_log_ctx.w_pos > 0) {
                _file_log_ctx.w_size += fwrite(_file_log_ctx.buf, 1,
                        _file_log_ctx.w_pos, _file_log_ctx.fp);
                _file_log_ctx.w_pos = 0;
            }
            _file_log_ctx.w_size += fwrite(head, 1, h_size, _file_log_ctx.fp);
            _file_log_ctx.w_size += fwrite(log, 1, m_size, _file_log_ctx.fp);
        }else{
            memcpy(_file_log_ctx.buf, head, h_size);
            _file_log_ctx.w_pos += h_size;
            _file_log_ctx.w_size += h_size;
            memcpy(_file_log_ctx.buf, log, m_size);
            _file_log_ctx.w_pos += m_size;
            _file_log_ctx.w_size += m_size;
        }

        int sw_log = 0;

        if (_file_log_ctx.switch_time > 0) {
            struct timeval tv = { 0 };
            gettimeofday(&tv, NULL );
            struct tm* tm = localtime(&tv.tv_sec);

            int seconds = tm->tm_hour * 60 + tm->tm_sec;
            if(seconds >= _file_log_ctx.switch_time){
                if(!_file_log_ctx.switch_flag) {
                    sw_log = 1;
                    _file_log_ctx.switch_flag = 1;
                }
            }else{
                _file_log_ctx.switch_flag = 0;
            }
        }
        if(!sw_log) {
            if(_file_log_ctx.switch_size > 0 &&
                    _file_log_ctx.w_size >= _file_log_ctx.switch_size) {
                sw_log = 1;
            }
        }
        if(sw_log) {

            fclose(_file_log_ctx.fp);
            _file_log_ctx.fp = NULL;
            _file_log_ctx.file_count++;
            if(log_file_init_callback_fun(&_file_log_ctx) != LOG_SUCCESS) {
                return LOG_FAIL;
            }
        }
    }
    return LOG_SUCCESS;
}
int log_file_uninit_callback_fun(void* args)
{
    FILE* fp = _file_log_ctx.fp;
    char* buf = _file_log_ctx.buf;
    if(fp) {
        fclose(fp);
    }
    if(buf) {
        free(buf);
    }
    return LOG_SUCCESS;
}
