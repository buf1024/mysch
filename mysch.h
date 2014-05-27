/*
 * mysch.h
 *
 *  Created on: 2014-5-27
 *      Author: Luo Guochun
 */

#ifndef __MYSCH_H__
#define __MYSCH_H__

#include "myqueue.h"
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
              " section: %s, item: %s\n", s->key, k);     \
            return -1;                                    \
        }else{                                            \
            v = atoi(t);                                  \
        }                                                 \
    }while(0)

#define READ_CONF_STR_MUST(s, k, v)                       \
    do{                                                   \
        const char* t = ini_get_val(s, k);                \
        if(NULL == t){                                    \
            printf("fail to obtain configuration item."   \
              " section: %s, item: %s\n", s->key, k);     \
            return -1;                                    \
        }else{                                            \
            /*必须确保V比T空间大*/                           \
            strcpy(v, t);                                 \
        }                                                 \
    }while(0)


typedef struct mprog_s mprog_t;

struct mprog_s
{
    char command[256];
    char group[128];
    char user[64];

    int keepalive;
    int waitfinish;
};

typedef struct sch_info_s sch_info_t;

struct sch_info_s
{
    list_t* progq;
    dict* waitd;
};

#endif /* __MYSCH_H__ */
