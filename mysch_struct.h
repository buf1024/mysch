/*
 * mysch_struct.h
 *
 *  Created on: 2014-5-29
 *      Author: Luo Guochun
 */

#ifndef __MYSCH_STRUCT_H__
#define __MYSCH_STRUCT_H__

#include "dlist.h"
#include "dict.h"

#define REGISTER_SIGNAL(signo, func, interupt)            \
    do {                                                  \
        if(set_signal(signo, func, interupt) == SIG_ERR){ \
            printf("register signal %d failed:%s\n",      \
                    signo, strerror(errno));              \
            return -1;                                    \
        }                                                 \
    }while(0)


#define READ_CONF_INT_MUST(s, k, v)                       \
    do{                                                   \
        const char* t = ini_get_val(s, k);                \
        if(NULL == t){                                    \
            printf("fail to obtain configuration item."   \
              " item: %s\n", k);                          \
            return -1;                                    \
        }else{                                            \
            v = atoi(t);                                  \
        }                                                 \
    }while(0)


#define READ_CONF_INT_OPT(s, k, v)                        \
    do{                                                   \
        const char* t = ini_get_val(s, k);                \
        if(NULL != t){                                    \
            v = atoi(t);                                  \
        }                                                 \
    }while(0)


#define READ_CONF_STR_MUST(s, k, v)                       \
    do{                                                   \
        const char* t = ini_get_val(s, k);                \
        if(NULL == t){                                    \
            printf("fail to obtain configuration item."   \
              " item: %s\n", k);                          \
            return -1;                                    \
        }else{                                            \
            /*必须确保V比T空间大*/                           \
            strcpy(v, t);                                 \
        }                                                 \
    }while(0)


#define READ_CONF_STR_OPT(s, k, v)                        \
    do{                                                   \
        const char* t = ini_get_val(s, k);                \
        if(NULL != t){                                    \
            /*必须确保V比T空间大*/                           \
            strcpy(v, t);                                 \
        }                                                 \
    }while(0)

typedef struct prog_s prog_t;
typedef struct cond_s cond_t;
typedef struct sch_info_s sch_info_t;

typedef struct list   cond_group_t;
typedef struct list   prog_group_t;
typedef struct dict   prog_wait_t;

struct prog_s
{
    pid_t pid;
    char cmd[256];
    int argc;
    char argv[16][128];
    char user[64];

    int flag;
    cond_group_t* cond;

    int status;
    long time;
};

struct cond_s
{
    char day[7 + 1];
    long start;
    long end;
};

struct sch_info_s
{
    int sleep_time;
    int kill_flag;
    char conf[256];
    prog_group_t* progq;
    prog_wait_t* waitq;
};


#endif /* __MYSCH_STRUCT_H__ */
