/*
 * main.c
 *
 *  Created on: 2014-5-28
 *      Author: Luo Guochun
 */

#define C_TEST_APP

#include "ctest.h"

int main(int argc, char **argv)
{
    INIT_TEST(argc, argv);
    RUN_ALL_TEST();

    return 0;
}

