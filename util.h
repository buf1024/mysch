/*
 * util.h
 *
 *  Created on: 2014-1-23
 *      Author: Luo Guochun
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <time.h>

#define DATE_TIME_FORMAT_1 "%04d%02d%02d%02d%02d%02d"
#define DATE_TIME_FORMAT_2 "%04d-%02d-%02d %02d:%02d:%02d"
#define DATE_FORMAT_1      "%04d%02d%02d"
#define DATE_FORMAT_2      "%04d-%02d-%02d"
#define TIME_FORMATE_1     "%02d%02d%02d"
#define TIME_FORMATE_2     "%02d:%02d:%02d"
#define TIME_FORMATE_3     "%02d-%02d-%02d"

typedef void sigfunc(int);
sigfunc* set_signal(int signo, sigfunc* func, int interupt);

int make_daemon();
int is_prog_runas_root();
int is_prog_running(const char* name);
int runas(const char* user);

time_t get_1970_sec();
time_t get_1970_usec();
time_t get_1970_ms();

time_t get_utc_sec();
time_t get_utc_usec();
time_t get_utc_ms();

time_t get_1970_sec_0_am();
time_t get_1970_usec_0_am();
time_t get_1970_ms_0_am();

int sec_1970_to_datetime(time_t sec, const char* fmt, char* datetime, int len);
int sec_1970_to_date(time_t sec, const char* fmt, char* time, int len);
int sec_1970_to_time(time_t sec, const char* fmt, char* time, int len);

int sec_1970_to_numeric(time_t s,
        int* year, int* mon, int* day,
        int* hour, int* min, int* sec);

int match(const char* text, const char* pattern);

int split(const char* text, char needle, char** dest, int size, int num);

const char* trim_left(const char* text, const char* needle, char* dest, int size);
const char* trim_right(const char* text, const char* needle, char* dest, int size);
const char* trim(const char* text, const char* needle, char* dest, int size);

#endif /* __UTIL_H__ */
