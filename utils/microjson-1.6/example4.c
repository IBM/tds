/* example4.c - fourth example in the programming guide
 *
 * This file is Copyright (c) 2020 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mjson.h"

#define ARR1_LENGTH 8

static bool flag1;
static int arr1[ARR1_LENGTH];
static int arr1_count;

const struct json_attr_t json_attrs_example4[] = {
    {"flag1",      t_boolean,	.addr.boolean = &flag1},
    {"arr1",       t_array,	.addr.array.element_type = t_integer,
				.addr.array.arr.integers = arr1,
				.addr.array.maxlen = ARR1_LENGTH,
				.addr.array.count = &arr1_count},
    {NULL},
};

int main(int argc, char *argv[])
{
    const char* end = (const char*) argv[1] + strlen((const char*) argv[1]);
    const char* cur = (const char*) argv[1];

    while (cur < end) {
	int status = json_read_object(cur, json_attrs_example4, &cur);
	printf("status: %d, flag1: %d\n", status, flag1);
	for (int i = 0; i < arr1_count; i++)
	    printf("arr1 = %d\n", arr1[i]);
        if (status != 0)
	    puts(json_error_string(status));
	arr1_count = 0;
    }
}

/* end */
