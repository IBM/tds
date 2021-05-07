/* example2.c - second example in the programming guide
 *
 * This file is Copyright (c) 2014 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <stdlib.h>

#include "mjson.h"

#define MAXCHANNELS 72

static bool usedflags[MAXCHANNELS];
static int PRN[MAXCHANNELS];
static int elevation[MAXCHANNELS];
static int azimuth[MAXCHANNELS];
static int visible;

const struct json_attr_t sat_attrs[] = {
    {"PRN",	t_integer, .addr.integer = PRN},
    {"el",	t_integer, .addr.integer = elevation},
    {"az",	t_integer, .addr.integer = azimuth},
    {"used",	t_boolean, .addr.boolean = usedflags},
    {NULL},
};

const struct json_attr_t json_attrs_sky[] = {
    {"class",      t_check,   .dflt.check = "SKY"},
    {"satellites", t_array,   .addr.array.element_type = t_object,
		   	      .addr.array.arr.objects.subtype=sat_attrs,
			      .addr.array.maxlen = MAXCHANNELS,
			      .addr.array.count = &visible},
    {NULL},
    };

int main(int argc, char *argv[])
{
    int i, status = 0;

    status = json_read_object(argv[1], json_attrs_sky, NULL);
    printf("%d satellites:\n", visible);
    for (i = 0; i < visible; i++)
	printf("PRN = %d, elevation = %d, azimuth = %d\n", 
	       PRN[i], elevation[i], azimuth[i]);

    if (status != 0)
	puts(json_error_string(status));
}

/* end */
