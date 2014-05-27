/*
 * mysch.c
 *
 *  Created on: 2014-5-27
 *      Author: Luo Guochun
 */


#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "mysch.h"
#include "util.h"
#include "iniconf.h"
#include "log.h"


int g_exit = 0;
int g_usr2 = 0;
int g_sleep_time = 0;
sch_info_t g_info = { 0 };

static void sigterm(int signo)
{
    g_exit = 1;
}

static void sigusr2(int signo)
{
    g_usr2 = 1;
}

static void sigchild(int signo)
{
    int status = 0;
    pid_t pid = wait(&status);

    mprog_t* prg = dict_fetch_value(g_info.waitd, &pid, sizeof(pid_t));
    if(prg == NULL) {
        LOG_ERROR("can't find context of exit pid: %d, exit: %d\n", pid, status);
        return;
    }

    if(prg->keepalive == 0 ) {
        dict_delete(g_info.waitd, &pid, sizeof(pid_t));
        LOG_INFO("prog %s exit, exitcode: %d\n", prg->command, status);
    }else{
        LOG_WARN("keepalive prog %s exit, exitcode: %d\n", prg->command, status);
    }
}

//◊¢≤·–≈∫≈
static int reg_sign()
{
    REGISTER_SIGNAL(SIGTERM, sigterm, 0);
    //Kill–≈∫≈
    REGISTER_SIGNAL(SIGINT, sigterm, 0);
    //÷’∂ÀCTRL-C–≈∫≈
    REGISTER_SIGNAL(SIGUSR1, SIG_IGN, 0);
    //∫ˆ¬‘SIGUSR1–≈∫≈
    REGISTER_SIGNAL(SIGUSR2, sigusr2, 0);
    //∫ˆ¬‘SIGUSR2–≈∫≈
    REGISTER_SIGNAL(SIGHUP, SIG_IGN, 0);
    //∫ˆ¬‘SIGHUP–≈∫≈
    REGISTER_SIGNAL(SIGCHLD, sigchild, 0);
    //∫ˆ¬‘◊”Ω¯≥ÃÕÀ≥ˆ
    REGISTER_SIGNAL(SIGPIPE, SIG_IGN, 0);
    //∫ˆ¬‘SIGPIPE–≈∫≈
    REGISTER_SIGNAL(SIGALRM, SIG_IGN, 0);
    //∫ˆ¬‘SIGALRM–≈∫≈

    return 0;
}

static int init_context(sch_info_t* info)
{
    info->progq = list_create();
    info->waitd = dict_create(NULL);



    return 0;
}

static void uninit_context(sch_info_t* info)
{
    if(info->progq) {
        list_destroy(info->progq);
    }
    if(info->waitd) {
        dict_release(info->waitd);
    }

    LOG_FINISH();
}

static int load_conf(const char* conf)
{
    int ret = 0;
    if (NULL == conf) {
        return -1;
    }
    ini_conf_t* ini = ini_create();
    ret = ini_load(ini, conf);
    if (ret != 0) {
        ini_destroy(ini);
        printf("ini_load failed.\n");
        return -1;
    }

    ini_sec_t* sec = ini_get_sec(ini, "COMMON");
    if(sec == NULL) {
        printf("ini_get_sec failed.\n");
        ini_destroy(ini);
        return -1;
    }

    {
        char term_level[64] = { 0 };
        char file_level[64] = { 0 };
        char log_path[256] = { 0 };
        char log_header[64] = { 0 };
        int log_buffer = 1024;

        READ_CONF_STR_MUST(sec, "LOG_TERM_LEVEL", term_level);
        READ_CONF_STR_MUST(sec, "LOG_FILE_LEVEL", file_level);
        READ_CONF_STR_MUST(sec, "LOG_FILE_PATH", log_path);
        READ_CONF_STR_MUST(sec, "LOG_HEADER", log_header);
        READ_CONF_INT_MUST(sec, "LOG_BUFFER", log_buffer);

        LOG_INITIALIZE_DEFAULT(log_get_level(term_level),
                log_get_level(file_level), log_path, log_header,
                log_buffer, -1, -1
                );
    }

    READ_CONF_INT_MUST(sec, "SLEEP_TIME", g_sleep_time);

    {
        char prog_list[2048] = { 0 };
        char progs[128][64] = { { 0 } };
        int num = 0;
        READ_CONF_STR_MUST(sec, "PROG_LIST", prog_list);

        num = split(prog_list, '|', (char **)progs, 64, 128);
        if(num <= 0) {
            printf("split %s failed.\n", prog_list);
            ini_destroy(ini);
            return -1;
        }

        int i = 0;
        for(i=0; i<num; i++) {

            ini_sec_t* ssec = ini_get_sec(ini, progs[i]);
            if(ssec == NULL) {
                printf("get section %s failed.\n", progs[i]);
                ini_destroy(ini);
                return -1;
            }

            mprog_t* prog = (mprog_t*)malloc(sizeof(*prog));

            READ_CONF_STR_MUST(ssec, "COMMAND", prog->command);
            READ_CONF_STR_MUST(ssec, "GROUP", prog->group);
            READ_CONF_STR_MUST(ssec, "USER", prog->user);
            READ_CONF_INT_MUST(ssec, "KEEPALIVE", prog->keepalive);
            READ_CONF_INT_MUST(ssec, "WAITFINISH", prog->waitfinish);

            list_node_t* node = list_node_create(prog, free);

            if(prog->waitfinish == 0) {
                list_insert_head(g_info.progq, node);
            }else{
                int     flag = 0;
                list_node_t* pre = NULL;
                list_node_t* it = NULL;
                for(it = list_first(g_info.progq);
                        !list_empty(g_info.progq); it = list_next(it)) {
                    mprog_t* data = (mprog_t*)list_node_data(it);
                    if(data->keepalive) {
                        list_insert_before(it, node);
                        flag = 1;
                        break;
                    }
                    pre = it;
                }
                if(!flag) {
                    if(!pre) {
                        list_insert_head(g_info.progq, node);
                    }else{
                        list_insert_after(pre, node);
                    }
                }
            }
        }

        LOG_DEBUG("dump prog\n");
        list_node_t* it = NULL;
        for(it = list_first(g_info.progq);
                !list_empty(g_info.progq); it = list_next(it)) {
            mprog_t* prg = list_node_data(it);
            LOG_DEBUG("\ncommand: %s, group: %s, user: %s, keepalive: %d, waitfinish: %d\n",
                    prg->command, prg->group, prg->user, prg->keepalive, prg->waitfinish);
        }
        LOG_DEBUG("end dump prog\n");

    }


    return 0;
}

static void usage()
{
    printf("./myshc [-c config] [-h]\n");
    printf("  -c config    load configuration file.\n");
    printf("  -h           show this help message.\n");
}

int main(int argc, char* argv[])
{
    extern char *optarg;
    int optch;
    char optstring[] = "c:h";

    char* conf = NULL;

    //∂¡»°√¸¡Ó––≤Œ ˝
    while ((optch = getopt(argc, argv, optstring)) != -1) {
        switch (optch) {
        case 'h':
            usage();
            exit(0);
        case 'c':
            conf = optarg;
            break;
        default:
            usage();
            exit(0);
        }
    }

    if (conf == NULL || conf[0] == 0) {
        printf("configuration file is empty.\n");
        usage();
        exit(0);
    }

    reg_sign();

    init_context(&g_info);

    if(load_conf(conf) != 0) {
        printf("load_conf failed.\n");
        exit(-1);
    }

    make_daemon();

    while(!g_exit){

    }

    uninit_context(&g_info);

    return 0;
}
