/*
 * log_test.c
 *
 *  Created on: 2014-5-28
 *      Author: Luo Guochun
 */

#include "ctest.h"
#include "log.h"

TEST(log, log)
{
    log_initialize_default(LOG_DEBUG, LOG_DEBUG,
            "./", "test_log_1", 64, -1, -1);
    int i = 0;
    for (; i < 100; i++) {
        LOG_DEBUG("hello %s\n", "lgc");
        LOG_DEBUG("hello %lf\n", 0.001);
    }
    log_finish();

    log_initialize_default(LOG_DEBUG, LOG_DEBUG,
        "./", "test_log_2", 64, 750, -1);

    for (i = 0 ; i < 100; i++) {
        LOG_DEBUG("hello %s\n", "cnb");
        LOG_DEBUG("hello %lf\n", 0.002);
    }
    log_finish();
}
