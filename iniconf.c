/*
 * iniconf.c
 *
 *  Created on: 2014-1-22
 *      Author: Luo Guochun
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "iniconf.h"
#include "util.h"

typedef struct ini_kv_s ini_kv_t;

struct ini_kv_s
{
    char k[INI_MAX_KEY_LEN];
    char v[INI_MAX_VALUE_LEN];
};


static int read_line(int fd, char* buf, int size)
{
    int index = 0;
    int rd = 0;
    char ch;

    memset(buf, 0, size);

    while((rd = read(fd, &ch, 1)) > 0){
        if(ch != '\n'){
            if(index <= size){
                buf[index++] = ch;
            }else{
                return -1;
            }
        }else{
            break;
        }
    }
    if(index > 0){
        if(buf[index - 1] == '\r'){
            buf[index - 1] = 0;
            index--;
        }
    }
    if(rd <= 0){
        return -1;
    }

    return index;
}
static int ini_save_sec(ini_sec_t* sec, int fd)
{
    if (!sec || fd < 0)
        return -1;

    dict_iterator* it = dict_get_iterator(sec);
    dict_entry* de = NULL;
    while((de = dict_next(it)) != NULL) {
        const char* k = (const char*)dict_get_entry_key(de);
        const char* v = (const char*)dict_get_entry_val(de);


        write(fd, k, strlen(k));
        write(fd, "=", 1);
        write(fd, v, strlen(v));
        write(fd, "\n", 1);
    }
    dict_release_iterator(it);
    return 0;
}
static int ini_save_sec_buf(ini_sec_t* sec, char* buf, size_t len)
{
    if (!sec || !buf || len <= 0)
        return -1;

    size_t wlen = 0;
    dict_iterator* it = dict_get_iterator(sec);
    dict_entry* de = NULL;
    while((de = dict_next(it)) != NULL) {
        const char* k = (const char*)dict_get_entry_key(de);
        const char* v = (const char*)dict_get_entry_val(de);

        wlen += snprintf(buf + wlen, len - wlen, "%s", k);
        if(wlen >= len) return -1;
        wlen += snprintf(buf + wlen, len - wlen, "=");
        if(wlen >= len) return -1;
        wlen += snprintf(buf + wlen, len - wlen, "%s", v);
        if(wlen >= len) return -1;
        wlen += snprintf(buf + wlen, len - wlen, "\n");
        if(wlen >= len) return -1;
    }
    dict_release_iterator(it);
    return wlen;
}
int ini_destroy(ini_conf_t* ini)
{
    dict_iterator* it = dict_get_safe_iterator(ini);
    dict_entry* de = NULL;
    while((de = dict_next(it)) != NULL) {
        dict* v = (dict*)dict_get_entry_val(de);
        dict_delete_no_free(ini, de->key, de->keylen);

        free(de->key);

        dict_release(v);
    }
    dict_release_iterator(it);
    dict_release(ini);

    return 0;
}
ini_conf_t* ini_create()
{
    return dict_create(NULL);
}

int ini_load(ini_conf_t* ini, const char* file)
{
    if(!ini || !file) return -1;

    int fd = open(file, O_RDONLY);
    if (fd == -1) return -1;

    char tmpline[INI_READ_LINE + 1] = { 0 };
    char line[INI_READ_LINE + 1] = { 0 };
    char* tmp = 0;
    char key[INI_MAX_KEY_LEN] = { 0 };
    char value[INI_MAX_VALUE_LEN] = { 0 };

    char tmpkey[INI_MAX_KEY_LEN] = { 0 };
    char tmpvalue[INI_MAX_VALUE_LEN] = { 0 };

    ini_sec_t* cur_sec = 0;

    int len = 0;
    while (1) {
        if ((len = read_line(fd, tmpline, INI_READ_LINE)) < 0) {
            break;
        }
        if (*tmpline == '#' || *tmpline == ';' || len == 0) {
            continue;
        }
        trim(tmpline, " ", line, INI_READ_LINE);
        if(line[0] == 0) {
            continue;
        }

        // start section
        memset(key, 0, INI_MAX_KEY_LEN);
        memset(value, 0, INI_MAX_VALUE_LEN);
        memset(tmpkey, 0, sizeof(tmpkey));
        memset(tmpvalue, 0, sizeof(tmpvalue));

        tmp = line;
        if (*tmp == '[') {
            const char* key_del = ++tmp;
            while (*tmp != '\0' && *tmp != ']') {
                tmp++;
            }
            if (*tmp == '\0') {
                return -1;
            }
            strncpy(key, key_del, tmp - key_del);
            cur_sec = ini_insert_sec(ini, key);
            continue;
        }
        if (cur_sec) {
            const char* eq_del = tmp;
            while (*eq_del != '\0') {
                if (*eq_del == '=') {
                    break;
                }
                eq_del++;
            }
            if (*eq_del == '\0' || *(eq_del + 1) == '\0') {
                continue;
            }

            strncpy(tmpkey, tmp, eq_del - tmp);
            strcpy(tmpvalue, eq_del + 1);

            trim(tmpkey, " ", key, INI_MAX_KEY_LEN);
            trim(tmpvalue, " ", value, INI_MAX_VALUE_LEN);

            if(key[0] == 0 || value[0] == 0) {
                continue;
            }

            ini_insert_val(cur_sec, key, value);

        }
    }
    close(fd);
    return 0;
}
int ini_save(ini_conf_t* ini, const char* file)
{
    if (!ini || !file)
        return -1;

    int fd = open(file, O_RDWR | O_CREAT,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd == -1)
        return -1;

    dict_iterator* it = dict_get_iterator(ini);
    dict_entry* de = NULL;
    while((de = dict_next(it)) != NULL) {
        const char* k = (const char*)dict_get_entry_key(de);
        ini_sec_t* v = (ini_sec_t*)dict_get_entry_val(de);


        write(fd, "[", 1);
        write(fd, k, strlen(k));
        write(fd, "]", 1);
        write(fd, "\n", 1);

        int ret = ini_save_sec(v, fd);
        if(ret < 0){
            close(fd);
            return -1;
        }
    }
    dict_release_iterator(it);
    close(fd);
    return 0;
}

int ini_dump(ini_conf_t* ini, char* buf, size_t len)
{
    if (!ini || !buf || !ini)
        return -1;
    size_t wlen = 0;
    dict_iterator* it = dict_get_iterator(ini);
    dict_entry* de = NULL;
    while((de = dict_next(it)) != NULL) {
        const char* k = (const char*)dict_get_entry_key(de);
        ini_sec_t* v = (ini_sec_t*)dict_get_entry_val(de);

        if(wlen >= len) return -1;

        wlen += snprintf(buf + wlen, len - wlen, "[%s]\n", k);
        if(wlen >= len) return -1;

        int ret = ini_save_sec_buf(v, buf + wlen, len - wlen);
        if(ret <= 0) return -1;
        wlen += ret;
    }
    dict_release_iterator(it);
    return 0;
}

ini_sec_t* ini_insert_sec(ini_conf_t* ini, const char* key)
{
    if (!ini || !key)
        return NULL ;

    int len = strlen(key);
    char* k = (char*)malloc(len + 1);
    strcpy(k, key);

    dict* v = dict_create(NULL);

    if(dict_add(ini, k, len, v, sizeof(v)) == DICT_OK){
        return v;
    }

    dict_release(v);

    return NULL;
}
int ini_delete_sec(ini_conf_t* ini, const char* key)
{
    if (!ini || !key)
        return -1 ;

    int len = strlen(key);

    dict* v = (dict*)dict_fetch_value(ini, key, len);

    if(!v) {
        return -1;
    }

    dict_release(v);
    dict_delete(ini, key, len);

    return 0;
}
ini_sec_t* ini_get_sec(ini_conf_t* ini, const char* key)
{
    if (!ini || !key)
        return NULL ;

    int len = strlen(key);

    dict* v = (dict*)dict_fetch_value(ini, key, len);

    return v;
}

int ini_insert_val(ini_sec_t* sec, const char* key, const char* val)
{
    if (!sec || !key || !val)
        return -1 ;

    int klen = strlen(key);
    char* k = (char*)malloc(klen + 1);
    strcpy(k, key);

    int vlen = strlen(val);
    char* v = (char*)malloc(vlen + 1);
    strcpy(v, val);

    if(dict_add(sec, k, klen, v, vlen) == DICT_OK) {
        return 0;
    }

    return -1;
}
int ini_delete_val(ini_sec_t* sec, const char* key)
{
    if (!sec || !key)
        return -1 ;

    int klen = strlen(key);


    if(dict_delete(sec, key, klen) == DICT_OK) {
        return 0;
    }

    return -1;
}
const char* ini_get_val(ini_sec_t* sec, const char* key)
{
    if (!sec || !key)
        return NULL ;

    int klen = strlen(key);

    char* v = (char*)dict_fetch_value(sec, key, klen);

    return v;
}
