/* example1.c - first example in the programming guide
 *
 * This file is Copyright (c) 2014 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mjson.h"

static bool flag1, flag2;
static int count;

static const struct json_attr_t json_attrs[] = {
    {"count",   t_integer, .addr.integer = &count},
    {"flag1",   t_boolean, .addr.boolean = &flag1,},
    {"flag2",   t_boolean, .addr.boolean = &flag2,},
    {NULL},
};

int main(int argc, char *argv[])
{
    int status = 0;

    status = json_read_object(argv[1], json_attrs, NULL);
    printf("status = %d, count = %d, flag1 = %d, flag2 = %d\n",
	   status, count, flag1, flag2);
    if (status != 0)
	puts(json_error_string(status));
}

/* end */
