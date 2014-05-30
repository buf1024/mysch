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
#include <sys/time.h>

#include "mysch.h"
#include "util.h"
#include "iniconf.h"
#include "log.h"

//信号
static int g_term = 0;
static int g_usr1 = 0;
static int g_usr2 = 0;
static int g_chld = 0;

sch_info_t g_info = { 0 };

static void sigterm(int signo)
{
    g_term = 1;
}

static void sigusr1(int signo)
{
    g_usr1 = 1;
}

static void sigusr2(int signo)
{
    g_usr2 = 1;
}

static void sigchild(int signo)
{
    g_chld = 1;
}

//注册信号
int reg_sign()
{
    REGISTER_SIGNAL(SIGTERM, sigterm, 0);//Kill信号
    REGISTER_SIGNAL(SIGINT, sigterm, 0);//终端CTRL-C信号
    REGISTER_SIGNAL(SIGUSR1, sigusr1, 0);//SIGUSR1信号
    REGISTER_SIGNAL(SIGUSR2, sigusr2, 0);//SIGUSR2信号
    REGISTER_SIGNAL(SIGHUP, SIG_IGN, 0);//忽略SIGHUP信号
    REGISTER_SIGNAL(SIGCHLD, sigchild, 0);//子进程退出
    REGISTER_SIGNAL(SIGPIPE, SIG_IGN, 0);//忽略SIGPIPE信号
    REGISTER_SIGNAL(SIGALRM, SIG_IGN, 0);//忽略SIGALRM信号

    return 0;
}

int init_context(sch_info_t* info)
{
    info->progq = list_create();
    info->waitq = dict_create(NULL);

    return 0;
}

void uninit_context(sch_info_t* info)
{
    if(info->progq) {
        list_iter* it = list_get_iterator(info->progq, AL_START_HEAD);
        list_node* n = NULL;
        while((n = list_next(it)) != NULL) {
            prog_t* p = list_node_value(n);
            if(p->cond) {
                list_release(p->cond, AL_FREE);
            }
        }

        list_release_iterator(it);
        list_release(info->progq, AL_FREE);
    }
    if(info->waitq) {
        dict_release(info->waitq);
    }
}

int load_conf(sch_info_t* info, int flag)
{
    int ret = 0;
    ini_conf_t* ini = ini_create();
    ret = ini_load(ini, info->conf);
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

    READ_CONF_STR_OPT(sec, "RUN_USER", info->run_user);

    if(runas(g_info.run_user) != 0) {
        printf("runas %s failed.\n", g_info.run_user);
        return -1;
    }

    {
        char file_level[64] = { 0 };
        char log_path[256] = { 0 };
        char log_header[64] = { 0 };
        int log_buffer = 0;
        int log_sw_time = 0;

        READ_CONF_STR_MUST(sec, "LOG_LEVEL", file_level);
        READ_CONF_STR_MUST(sec, "LOG_PATH", log_path);
        READ_CONF_STR_MUST(sec, "LOG_HEADER", log_header);
        READ_CONF_INT_MUST(sec, "LOG_BUFFER", log_buffer);
        READ_CONF_INT_MUST(sec, "LOG_SWITCH_TIME", log_sw_time);

        if(log_buffer < 0 || log_sw_time < 0) {
            printf("log_buffer = %d or log_switch_time = %d invalid.\n",
                    log_buffer, log_sw_time);
            return -1;
        }
        if(log_sw_time > 86400) {
            log_sw_time %= 86400;
        }

        if (flag != LOAD_CONF_TEST) {
            if (flag == LOAD_CONF_RELOAD) {
                log_finish();
                log_clearup();
            }

            int lvl = log_get_level(file_level);

            if (log_initialize_console(lvl) != 0) {
                printf("log_initialize_console failed.\n");
                return -1;
            }

            if (log_initialize_file(lvl, log_path, log_header, log_buffer,
                    log_sw_time, -1) != 0) {
                printf("log_initialize_file failed.\n");
                return -1;
            }
            if (log_start() != 0) {
                printf("log_initilize failed.\n");
                return -1;
            }
            LOG_INFO("logger is ready.\n");
        }
    }

    READ_CONF_INT_MUST(sec, "SLEEP_TIME", info->sleep_time);
    READ_CONF_INT_MUST(sec, "KILL_CHILD_FLAG", info->kill_flag);
    READ_CONF_STR_OPT(sec, "PID_FILE", info->pid_file);

    {
        char prog_list[2048] = { 0 };
        char progs[128][64] = { { 0 } };
        int num = 0;
        READ_CONF_STR_MUST(sec, "PROG_LIST", prog_list);

        num = split(prog_list, '|', (char **)progs, 64, 128);
        if(num <= 0) {
            LOG_ERROR("split %s failed.\n", prog_list);
            ini_destroy(ini);
            log_finish();
            return -1;
        }

        if(flag == LOAD_CONF_RELOAD) {
            if(info->progq){
                list_iter* it = list_get_iterator(info->progq, AL_START_HEAD);
                list_node* n = NULL;
                while((n = list_next(it)) != NULL) {
                    prog_t* p = list_node_value(n);
                    if(p->cond) {
                        list_release(p->cond, AL_FREE);
                    }
                }

                list_release_iterator(it);
                list_release(info->progq, AL_FREE);
            }

            info->progq = list_create();
        }

        int i = 0;
        for(i=0; i<num; i++) {

            ini_sec_t* ssec = ini_get_sec(ini, progs[i]);
            if (ssec == NULL ) {
                LOG_ERROR("get section %s failed.\n", progs[i]);
                ini_destroy(ini);
                log_finish();
                return -1;
            }


            prog_t* prog = (prog_t*)malloc(sizeof(*prog));
            memset(prog, 0, sizeof(*prog));

            char run_cmd[1024] = {0};

            READ_CONF_STR_MUST(ssec, "RUN_COMMAND", run_cmd);
            READ_CONF_STR_MUST(ssec, "RUN_USER", prog->user);
            READ_CONF_INT_MUST(ssec, "RUN_FLAG", prog->flag);

            int ncmd = 0;
            ncmd = split(run_cmd, ' ',  (char**)prog->argv, 128, 16);
            if(ncmd <= 0){
                LOG_ERROR("split cmd %s failed\n", run_cmd);
                free(prog);

                ini_destroy(ini);
                log_finish();
                return -1;
            }
            prog->argc = ncmd;
            strcpy(prog->cmd, prog->argv[0]);

            if(prog->flag > 0) {
                char cond[1024] = {0};
                READ_CONF_STR_MUST(ssec, "RUN_TIME", cond);

                if(build_condition(prog, cond) != 0) {
                    LOG_ERROR("build_condition failed. condition=%s\n", cond);
                    free(prog);

                    ini_destroy(ini);
                    log_finish();
                    return -1;
                }
                READ_CONF_STR_OPT(ssec, "RUN_PID_FILE", prog->pid_file);
            }


            list_iter* it = list_get_iterator(info->progq, AL_START_HEAD);
            list_node* node = NULL;
            while((node = list_next(it))) {
                prog_t* lp = list_node_value(node);
                if(lp->flag > prog->flag) {
                    break;
                }
            }
            if(node == NULL) {
                if(list_length(info->progq) == 0) {
                    list_add_node_head(info->progq, prog);
                }else{
                    list_add_node_tail(info->progq, prog);
                }
            }else{
                list_insert_node(info->progq, node, prog, 0);
            }
            list_release_iterator(it);
        }

        list_iter* it = list_get_iterator(info->progq, AL_START_HEAD);
        list_node* node = NULL;
        while((node = list_next(it))) {
            prog_t* lp = list_node_value(node);
            LOG_DEBUG("prog = %s\n", lp->cmd);
        }
        list_release_iterator(it);
    }


    return 0;
}

void usage()
{
    printf("./myshc [-c config] [-h]\n");
    printf("--simple program scheduler\n\n");
    printf("  -c config    load configuration file.\n");
    printf("  -e           don't run as daemon process.\n");
    printf("  -t           test configuration file.\n");
    printf("  -h           show this help message.\n");
}

int judge_condition(prog_t* prog)
{
    if(prog->flag <= 0 || !prog->cond) {
        return 1;
    }

    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);

    int wday = tm->tm_wday;
    if(wday == 0) {
        wday = 7; // 星期天
    }
    int sec = tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;

    list_iter* it = list_get_iterator(prog->cond, AL_START_HEAD);
    list_node* node = NULL;
    while((node = list_next(it)) != NULL) {
        cond_t* c = list_node_value(node);

        if(c->day[wday] == 1) {
            if(sec >= c->start && sec <= c->end) {
                list_release_iterator(it);
                return 1;
            }
        }
    }

    list_release_iterator(it);

    return 0;
}
int build_condition(prog_t* prog, const char* cond)
{
    char daytimes[128][64] = { { 0 } };
    int num = 0;

    LOG_DEBUG("condition: %s\n", cond);
    num = split(cond, '|', (char **) daytimes, 64, 128);
    if (num <= 0) {
        LOG_ERROR("split %s failed.\n", cond);
        return -1;
    }

    prog->cond = list_create();

    int fail = 0;
    int i = 0;
    for (i = 0; i < num; i++) {
        char daytime[2][64] = { { 0 } };
        int dtnum = 0;

        dtnum = split(daytimes[i], ',', (char**)daytime, 64, 2);
        if(dtnum != 2) {
            LOG_ERROR("split %s failed.\n", daytimes[i]);
            fail = 1;
            break;
        }

        if(strlen(daytime[0]) != 3 || daytime[0][1] != '-') {
            LOG_ERROR("day time format not ritht: day = %s\n", daytime[0]);
            fail = 1;
            break;
        }
        int d_start = atoi(daytime[0]);
        int d_end = atoi(daytime[0] + 2);

        if(d_start > d_end || d_start <= 0 || d_end <= 0 ||
                d_start > 7 || d_end > 7) {
            LOG_ERROR("day format not ritht: day = %s\n", daytime[0]);
            fail = 1;
            break;
        }

        char times[2][64] =  { { 0 } };
        int tnum = 0;
        tnum = split(daytime[1], '-', (char**)times, 64, 2);
        if(tnum != 2) {
            LOG_ERROR("split %s failed.\n", daytimes[i]);
            fail = 1;
            break;
        }
        if(strlen(times[0]) != 5 || times[0][2] != ':' ||
                strlen(times[1]) != 5 || times[1][2] != ':' ) {
            LOG_ERROR("time split format not ritht. time1=%s, times2=%s\n",
                    times[0], times[1]);
            fail = 1;
            break;
        }

        int t_start = atoi(times[0]) * 3600 + atoi(times[0] + 3) * 60;
        int t_end = atoi(times[1]) * 3600 + atoi(times[1] + 3) * 60;

        if(t_start >= t_end ||
                t_start < 0 || t_end < 0 ||
                t_start > 86400 || t_end > 86400) {
            LOG_ERROR("time data format not ritht. time1=%s, times2=%s, start=%d, end=%d\n",
                    times[0], times[1], t_start, t_end);
            fail = 1;
            break;
        }

        LOG_DEBUG("condition: daystart=%d, dayend=%d, timestart=%d, timeend=%d\n",
                d_start, d_end, t_start, t_end);

        cond_t* c = (cond_t*)malloc(sizeof(*c));
        memset(c, 0, sizeof(*c));

        int i = 0;
        for(i=d_start; i<=d_end; i++) {
            c->day[i] = 1;
        }
        c->start = t_start;
        c->end = t_end;

        list_add_node_tail(prog->cond, c);
    }

    if(fail) {
        list_release(prog->cond, AL_FREE);
        return -1;
    }

    return 0;
}

void handle_sigchild()
{
    pid_t pid;
    int status = 0;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        prog_t** pp = (prog_t**)dict_fetch_value(g_info.waitq,
                &pid, sizeof(pid));
        prog_t* p = *pp;

        p->status = status;
        p->time = time(NULL);

        if(dict_delete(g_info.waitq, &pid, sizeof(pid))!= DICT_OK) {
            LOG_ERROR("dict_delete failed. pid = %d\n", pid);
        }
    }
    g_chld = 0;
}
void handle_sigusr1()
{
    int ret = 0;

    ret = load_conf(&g_info, LOAD_CONF_RELOAD);
    if(ret != 0) {
        LOG_ERROR("reload config failed. exiting...\n");
    }else{
        LOG_INFO("prepare for restart program. killing...\n");
    }

    clean_proccess(1);

    g_usr1 = 0;
}

void clean_proccess(int killflag)
{
    dict_iterator* it = dict_get_iterator(g_info.waitq);
    dict_entry* de = NULL;
    while((de = dict_next(it)) != NULL) {
        pid_t* p = (pid_t*) dict_get_entry_key(de);
        if (killflag) {
            LOG_INFO("killing %d\n", *p);
            kill(*p, SIGTERM);
            waitpid(*p, NULL, 0);
            LOG_INFO("%d killed\n", *p);
        }else{
            prog_t* prog = *(prog_t**)dict_get_entry_val(de);
            LOG_INFO("program %s continue running even i die, pid: %d\n",
                    prog->cmd, *p);
        }

        dict_delete(g_info.waitq, p, sizeof(*p));
    }

    dict_release_iterator(it);
}

int run_prog(prog_t* prog)
{

    if(prog->flag > 0 &&
            prog->time != 0) {
        // 防止死循环，常驻进程启动不成功需等待
        time_t now = time(NULL);
        time_t diff = now - prog->time;
        if(diff <= g_info.sleep_time) {
            LOG_INFO("pause start program %s, timediff %d\n",
                    prog->cmd, diff);
            return -1;
        }
    }

    LOG_INFO("run program: %s\n", prog->cmd);
    int i = 0;
    for(i=0; i<prog->argc; i++) {
        LOG_DEBUG("arg%d=%s\n", i, prog->argv[i]);
    }
    pid_t p = fork();
    if(p == 0){
        int r = runas(prog->user);
        if(r == 0) {
            char* argv[prog->argc + 1];
            for(r = 0; r < prog->argc; r++) {
                argv[r] = prog->argv[r];
            }
            argv[prog->argc] = NULL;
            if(execv(prog->cmd, argv) != 0) {
                printf("execv %s failed. error: %s\n",
                        prog->cmd, strerror(errno));
                exit(-1);
            }
            exit(0);
        }
        exit(-1);
    }else{
        prog->pid = p;
        prog->update_pid = 0;

        pid_t* dp = (pid_t*)malloc(sizeof(dp));
        prog_t** dpp = (prog_t**)malloc(sizeof(dpp));

        *dp = p;
        *dpp = prog;

        dict_add(g_info.waitq, dp, sizeof(*dp), dpp, sizeof(*dpp));

        LOG_DEBUG("new proccess: %d\n", *dp);
    }
    return 0;
}

int update_pid(prog_t* prog)
{
    int ret = 0;
    if(!prog->update_pid) {
        if (prog->pid_file[0] != 0) {
            pid_t pid;
            if (read_pid_file(&pid, prog->pid_file) == 0) {
                if (kill(pid, 0) == 0) {
                    LOG_INFO("update pid from %d to %d\n", prog->pid, pid);

                    prog->pid = pid;
                    prog->update_pid = 1;

                    // 新进程信息

                    pid_t* dp = (pid_t*)malloc(sizeof(dp));
                    prog_t** dpp = (prog_t**)malloc(sizeof(dpp));

                    *dp = pid;
                    *dpp = prog;

                    dict_add(g_info.waitq, dp, sizeof(*dp), dpp, sizeof(*dpp));
                }
            } else {
                LOG_WARN("read_pid_file failed, pidfile = %s, error  = %s\n",
                        prog->pid_file, strerror(errno));
                ret = -1;
            }
        }
    }
    return ret;
}

int mytask()
{
    while(1) {
        sleep(g_info.sleep_time);

        if(g_usr1) {
            LOG_DEBUG("catch usr1\n");
            handle_sigusr1();
        }
        if(g_usr2) {
            LOG_DEBUG("catch usr2\n");
            log_flush();
            g_usr2 = 0;
        }
        if(g_chld) {
            LOG_DEBUG("catch child\n");
            handle_sigchild();
        }
        if(g_term) {
            LOG_DEBUG("catch term\n");
            break;
        }

        list_iter* it = list_get_iterator(g_info.progq, AL_START_HEAD);
        list_node* n = NULL;

        while((n = list_next(it)) != NULL) {
            prog_t* p = list_node_value(n);
            if(p->flag < 0) {
                if(p->pid == 0) {
                    // 启动程序
                    run_prog(p);
                    break;
                }
                if (kill(p->pid, 0) == 0) {
                    // 等待程序结束
                    break;
                }
                // 程序已经结束，继续
                list_del_node(g_info.progq, n, AL_FREE);

            }else if(p->flag == 0) {
                run_prog(p);
                // 程序已经启动，不管它生死存亡
                list_del_node(g_info.progq, n, AL_FREE);
            }else {
                prog_t* f = (prog_t*)
                        list_node_value(list_first(g_info.progq));
                if(f->flag < 0) {
                    // 等待程序结束
                    continue;
                }

                if (judge_condition(p)) {
                    if (p->pid > 0) {
                        if(update_pid(p) != 0) {
                            LOG_ERROR("update_pid failed.\n");
                            continue;
                        }
                        if (kill(p->pid, 0) == 0) {
                            // 程序正运行
                            continue;
                        }
                    }
                    run_prog(p);
                }else{
                    if (p->pid > 0) {
                        if (update_pid(p) != 0) {
                            LOG_ERROR("update_pid failed.\n");
                            continue;
                        }
                        if (kill(p->pid, 0) == 0) {
                            // 程序正运行,干掉它
                            LOG_INFO("killing %d\n", p->pid);
                            kill(p->pid, SIGTERM);
                            waitpid(p->pid, NULL, 0);
                            LOG_INFO("%d killed\n", p->pid);

                            dict_delete(g_info.waitq, &p, sizeof(pid_t));
                        }
                    }
                }
            }
        }

        list_release_iterator(it);
    }

    clean_proccess(g_info.kill_flag);

    return 0;
}

int main(int argc, char* argv[])
{
    extern char *optarg;
    int optch;
    char optstring[] = "c:htse";

    int test = 0;
    int daemon = 1;

    //读取命令行参数
    while ((optch = getopt(argc, argv, optstring)) != -1) {
        switch (optch) {
        case 'h':
            usage();
            exit(0);
        case 't':
            test = 1;
            break;
        case 'e':
            daemon = 0;
            break;
        case 'c':
            strcpy(g_info.conf, optarg);
            break;
        default:
            usage();
            exit(0);
        }
    }

    if (g_info.conf[0] == 0) {
        printf("configuration file is empty.\n");
        usage();
        exit(0);
    }

    if(test) {
        sch_info_t testinfo = {0};
        strcpy(testinfo.conf, g_info.conf);
        init_context(&testinfo);
        if(load_conf(&testinfo, LOAD_CONF_TEST) == 0) {
            printf("file syntax is correct.\n");
        }else{
            printf("file syntax is incorrect.\n");
        }
        uninit_context(&testinfo);
        exit(0);
    }

    if(daemon) {
        make_daemon();
    }

    reg_sign();

    init_context(&g_info);

    if(load_conf(&g_info, LOAD_CONF_NONE) != 0) {
        printf("load_conf failed.\n");
        uninit_context(&g_info);
        log_finish();
        exit(-1);
    }

    if(daemon) {
        if(g_info.pid_file[0] == 0) {
            LOG_ERROR("pid file is empty.\n");
            uninit_context(&g_info);
            log_finish();
            exit(-1);
        }
        if (write_pid_file(g_info.pid_file) != 0) {
            LOG_ERROR("write_pid_file failed.\n");
            uninit_context(&g_info);
            log_finish();
            exit(-1);
        }
    }


    mytask();

    uninit_context(&g_info);
    log_finish();

    return 0;
}
