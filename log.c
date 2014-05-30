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
#include <errno.h>

#define LOG_DEFAULT_HEAD_SIZE   128
#define LOG_DEFAULT_BUFFER_SIZE 4096
#define LOG_DEFAULT_MAX_PATH    256
//////////////////////////////////////////////////////////////////////
// built-in appender
// console/term appender
static int log_console_callback_log(const char* log, int len, void* args);

// file appender
static int log_file_init_callback_fun(void* args);
static int log_file_log_callback_fun(const char* log, int len, void* args);
static int log_file_uninit_callback_fun(void* args);
static int log_file_flush_callback_fun(void* args);

struct file_log
{
    char  path[LOG_DEFAULT_MAX_PATH];
    char  prefix[LOG_DEFAULT_MAX_PATH];
    char  name[LOG_DEFAULT_MAX_PATH];

    FILE* fp;
    int   file_count;
    int   switch_time;
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
    log_callback_log_t log_cb_fun;
    log_callback_uninit_t uninit_cb_fun;
    log_callback_uninit_t flush_cb_fun;
    void* args;

    struct call_back* next;
};

struct log_context
{
    struct call_back* cb;
};

static struct log_context    _context = {0};
static pthread_mutex_t     _ctx_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct file_log  _file_log_ctx = { {0} };

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
    int ret = snprintf(buf, size, "%s[%04d%02d%02d%02d%02d%02d:%ld]", head,
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec, (long)tv.tv_usec);

    return ret;
}

static void log_message(int lvl, const char* format, va_list ap)
{
    struct call_back* cb = _context.cb;

    int len = 0;
    char msg[LOG_DEFAULT_BUFFER_SIZE] = {0};

    while(cb){
        if (cb->log_cb_fun) {
            if (lvl >= cb->level) {
                if (!len) {
                    char head[LOG_DEFAULT_HEAD_SIZE] = { 0 };
                    log_get_header(lvl, head, LOG_DEFAULT_HEAD_SIZE);

                    char log[LOG_DEFAULT_BUFFER_SIZE] = { 0 };
                    vsnprintf(log, LOG_DEFAULT_BUFFER_SIZE, format, ap);

                    len = snprintf(msg, LOG_DEFAULT_BUFFER_SIZE, "%s%s", head, log);

                }

                if (cb->log_cb_fun(msg, len, cb->args) == LOG_FAIL) {
                    fprintf(stderr, "fail to log message!\n");
                }
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
        log_callback_int_t init_cb,
        log_callback_log_t log_cb,
        log_callback_uninit_t uninit_cb,
        log_callback_flush_t flush_cb,
        void* args)
{
    pthread_mutex_lock(&_ctx_mutex);

    struct call_back* cb = (struct call_back*)malloc(sizeof(struct call_back));
    if(cb){

        memset(cb, 0, sizeof(*cb));

        cb->level = lvl;//log_get_level(lvl);
        cb->init_cb_fun = init_cb;
        cb->log_cb_fun = log_cb;
        cb->uninit_cb_fun = uninit_cb;
        cb->flush_cb_fun = flush_cb;
        cb->args = args;
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
            if (cb->init_cb_fun(cb->args) == LOG_FAIL) {
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


void log_flush()
{

    pthread_mutex_lock(&_ctx_mutex);
    struct call_back* cb = _context.cb;
    while (cb) {
        if (cb->flush_cb_fun) {
            if (cb->flush_cb_fun(cb->args) == LOG_FAIL) {
                fprintf(stderr, "fail to flush log message!\n");
            }
        }
        cb = cb->next;
    }
    pthread_mutex_unlock(&_ctx_mutex);
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
    log_flush();

    pthread_mutex_lock(&_ctx_mutex);

    struct call_back* cb = _context.cb;
    while(cb){
        struct call_back* tmp_cb = cb;
        if (cb->uninit_cb_fun) {
            if (cb->uninit_cb_fun(cb->args) == LOG_FAIL) {
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

    if (log_initialize_console(con_lvl) == LOG_FAIL) {
        return LOG_FAIL;
    }

    if (log_initialize_file(file_lvl, file_path,
            prefix, buf_size, sw_time, sw_size) == LOG_FAIL) {
        return LOG_FAIL;
    }

    return log_start();
}

int log_initialize_console(int con_lvl)
{
    return log_reg_callback(con_lvl,
            0, log_console_callback_log, 0, 0,
            0);
}
int log_initialize_file(int file_lvl,
        char* file_path, char* prefix, int buf_size,
        int sw_time, int sw_size)
{
    if(!file_path || !prefix || buf_size <= 0) {
        return LOG_FAIL;
    }

    memset(&_file_log_ctx, 0, sizeof(_file_log_ctx));

    strncpy(_file_log_ctx.path, file_path, sizeof(_file_log_ctx.path) - 1);
    if(_file_log_ctx.path[strlen(_file_log_ctx.path) - 1] == '/') {
        _file_log_ctx.path[strlen(_file_log_ctx.path) - 1] = 0;
    }
    strncpy(_file_log_ctx.prefix, prefix, sizeof(_file_log_ctx.prefix) - 1);
    _file_log_ctx.buf_size = buf_size;
    _file_log_ctx.switch_size = sw_size;
    _file_log_ctx.switch_time = sw_time;
    _file_log_ctx.buf = malloc(sizeof(char)*buf_size);
    if(!_file_log_ctx.buf) {
        return LOG_FAIL;
    }
    memset(_file_log_ctx.buf, 0, sizeof(char)*buf_size);

    return log_reg_callback(file_lvl,
            log_file_init_callback_fun,
            log_file_log_callback_fun,
            log_file_uninit_callback_fun,
            log_file_flush_callback_fun,
            0);
}


/////////////////////////////////////
// built-in console appender
int log_console_callback_log(const char* log, int len, void* args)
{
    fprintf(stdout, log);
    return LOG_SUCCESS;
}

//built-in file appender
int log_file_init_callback_fun(void* args)
{
    FILE* fp = _file_log_ctx.fp;
    if(fp != NULL){
        if(_file_log_ctx.buf && _file_log_ctx.w_pos > 0) {
            fwrite(_file_log_ctx.buf, 1,
                    _file_log_ctx.w_pos, _file_log_ctx.fp);
        }

        fclose(fp);

        char old_path[LOG_DEFAULT_MAX_PATH] = {0};
        char new_path[LOG_DEFAULT_MAX_PATH] = {0};

        snprintf(old_path, LOG_DEFAULT_MAX_PATH, "%s/%s.tmp",
                _file_log_ctx.path, _file_log_ctx.name);

        snprintf(new_path, LOG_DEFAULT_MAX_PATH, "%s/%s.log",
                        _file_log_ctx.path, _file_log_ctx.name);

        if(rename(old_path, new_path) != 0) {
            fprintf(stderr, "rename %s - > %s failed: %s\n", old_path, new_path, strerror(errno));
        }

        _file_log_ctx.fp = NULL;
        memset(_file_log_ctx.name, 0, LOG_DEFAULT_MAX_PATH);
        _file_log_ctx.file_count += 1;
        _file_log_ctx.w_size = 0;
        _file_log_ctx.w_pos = 0;
    }

    char file[LOG_DEFAULT_MAX_PATH] = {0};
    pid_t pid = getpid();

    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);

    snprintf(_file_log_ctx.name, LOG_DEFAULT_MAX_PATH, "%s_%d_%04d%02d%2d_%d",
            _file_log_ctx.prefix, pid,
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            _file_log_ctx.file_count);

    snprintf(file, LOG_DEFAULT_MAX_PATH, "%s/%s.tmp",
            _file_log_ctx.path, _file_log_ctx.name);

    fp = fopen(file, "a+");
    if(fp == NULL){
        return LOG_FAIL;
    }
    _file_log_ctx.fp = fp;
    return LOG_SUCCESS;
}

int log_file_log_callback_fun(const char* log, int len, void* args)
{
    FILE* fp = _file_log_ctx.fp;
    if (fp == NULL ) {
        return LOG_FAIL;
    }
    int m_size = len;

    if (m_size > (_file_log_ctx.buf_size - _file_log_ctx.w_pos)) {
        if (_file_log_ctx.w_pos > 0) {
            _file_log_ctx.w_size += fwrite(_file_log_ctx.buf, 1,
                    _file_log_ctx.w_pos, _file_log_ctx.fp);
            _file_log_ctx.w_pos = 0;
        }
        _file_log_ctx.w_size += fwrite(log, 1, m_size, _file_log_ctx.fp);
    } else {
        memcpy(_file_log_ctx.buf + _file_log_ctx.w_pos, log, m_size);
        _file_log_ctx.w_pos += m_size;
        _file_log_ctx.w_size += m_size;
    }

    int sw_log = 0;

    if (_file_log_ctx.switch_time > 0) {
        struct timeval tv = { 0 };
        gettimeofday(&tv, NULL );
        struct tm* tm = localtime(&tv.tv_sec);

        int seconds = tm->tm_hour * 60 + tm->tm_sec;
        if (seconds >= _file_log_ctx.switch_time) {
            sw_log = 1;
        }
    }
    if (!sw_log) {
        if (_file_log_ctx.switch_size > 0
                && _file_log_ctx.w_size >= _file_log_ctx.switch_size) {
            sw_log = 1;
        }
    }
    if (sw_log) {

        if (log_file_init_callback_fun(&_file_log_ctx) != LOG_SUCCESS) {
            return LOG_FAIL;
        }
    }
    return LOG_SUCCESS;
}
int log_file_uninit_callback_fun(void* args)
{
    FILE* fp = _file_log_ctx.fp;
    char* buf = _file_log_ctx.buf;
    if(fp) {
        if(_file_log_ctx.buf && _file_log_ctx.w_pos > 0) {
            fwrite(_file_log_ctx.buf, 1,
                    _file_log_ctx.w_pos, _file_log_ctx.fp);
        }

        fclose(fp);

        char old_path[LOG_DEFAULT_MAX_PATH] = {0};
        char new_path[LOG_DEFAULT_MAX_PATH] = {0};

        snprintf(old_path, LOG_DEFAULT_MAX_PATH, "%s/%s.tmp",
                _file_log_ctx.path, _file_log_ctx.name);

        snprintf(new_path, LOG_DEFAULT_MAX_PATH, "%s/%s.log",
                        _file_log_ctx.path, _file_log_ctx.name);

        if(rename(old_path, new_path) != 0) {
            fprintf(stderr, "rename %s - > %s failed: %s\n", old_path, new_path, strerror(errno));
        }
    }
    if(buf) {
        free(buf);
    }
    memset(&_file_log_ctx, 0, sizeof(_file_log_ctx.buf));

    return LOG_SUCCESS;
}

int log_file_flush_callback_fun(void* args)
{
    FILE* fp = _file_log_ctx.fp;
    if(fp) {
        if(_file_log_ctx.buf && _file_log_ctx.w_pos > 0) {
            _file_log_ctx.w_size += fwrite(_file_log_ctx.buf, 1,
                    _file_log_ctx.w_pos, _file_log_ctx.fp);
            _file_log_ctx.w_pos = 0;

            fflush(fp);
        }
    }
    return LOG_SUCCESS;
}
