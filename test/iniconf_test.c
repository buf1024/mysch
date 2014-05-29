/*
 * iniconf_test.c
 *
 *  Created on: 2014-5-28
 *      Author: Luo Guochun
 */


#include "ctest.h"
#include "iniconf.h"

TEST(iniconf, iniconf)
{
    ini_conf_t* ini = ini_create();

    ini_load(ini, "./mysch.conf");
    char d[4000] = {0};
    ini_dump(ini, d, 4000);
    printf("%s\n", d);
    ini_save(ini, "./mysch2.conf");

    ini_sec_t* s = ini_get_sec(ini, "NGINX");
    ASSERT_TRUE(s != NULL);

    const char* c = ini_get_val(s, "COMMAND");
    ASSERT_TRUE(c != NULL);
    printf("COMMAND=%s\n", c);

    ini_destroy(ini);
}
