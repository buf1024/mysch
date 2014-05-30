/*
 * mysch.h
 *
 *  Created on: 2014-5-27
 *      Author: Luo Guochun
 */

#ifndef __MYSCH_H__
#define __MYSCH_H__

#include "mysch_struct.h"

enum {
    LOAD_CONF_TEST,
    LOAD_CONF_RELOAD,
    LOAD_CONF_NONE,
};

int reg_sign();

int judge_condition(prog_t* prog);
int build_condition(prog_t* prog, const char* cond);
void handle_sigchild();
void handle_sigusr1();

void clean_proccess(int killflag);

int reg_sign();
int init_context(sch_info_t* info);
void uninit_context(sch_info_t* info);

int load_conf(sch_info_t* info, int flag);
void usage();

int mytask();

int run_prog(prog_t* prog);
int update_pid(prog_t* prog);


#endif /* __MYSCH_H__ */
