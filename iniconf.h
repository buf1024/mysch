/*
 * iniconf.h
 *
 *  Created on: 2014-1-22
 *      Author: Luo Guochun
 */

#ifndef __INICONF_H__
#define __INICONF_H__

#include "myqueue.h"

/*this implementation don't care about the performance*/

#define INI_MAX_KEY_LEN  256
#define INI_MAX_VALUE_LEN 1024
#define INI_READ_LINE (INI_MAX_KEY_LEN + INI_MAX_VALUE_LEN + 2)

typedef struct ini_conf_s ini_conf_t;
typedef struct ini_sec_s ini_sec_t;

struct ini_sec_s
{
    char key[INI_MAX_KEY_LEN];
    slist_t* kv;
};

struct ini_conf_s
{
    slist_t* sec;
};


int ini_destroy(ini_conf_t* ini);
ini_conf_t* ini_create();

int ini_load(ini_conf_t* ini, const char* file);
int ini_save(ini_conf_t* ini, const char* file);

int ini_dump(ini_conf_t* ini, char* buf, size_t len);

ini_sec_t* ini_insert_sec(ini_conf_t* ini, const char* key);
int ini_delete_sec(ini_conf_t* ini, const char* key);
ini_sec_t* ini_get_sec(ini_conf_t* ini, const char* key);

const char* ini_sec_name(ini_sec_t* sec);

int ini_insert_val(ini_sec_t* sec, const char* key, const char* val);
int ini_delete_val(ini_sec_t* sec, const char* key);
const char* ini_get_val(ini_sec_t* sec, const char* key);

#endif /* __INICONF_H__ */
