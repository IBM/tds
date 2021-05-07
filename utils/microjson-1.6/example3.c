/* example2.c - first example in the programming guide
 *
 * This file is Copyright (c) 2014 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>

#include "mjson.h"

#define MAXUSERDEVS	4

struct devconfig_t {
    char path[PATH_MAX];
    double activated;
};

struct devlist_t {
    int ndevices;
    struct devconfig_t list[MAXUSERDEVS];
};

static struct devlist_t devicelist;

static int json_devicelist_read(const char *buf)
{
    const struct json_attr_t json_attrs_subdevice[] = {
	{"path",       t_string,     STRUCTOBJECT(struct devconfig_t, path),
	                                .len = sizeof(devicelist.list[0].path)},
	{"activated",  t_real,       STRUCTOBJECT(struct devconfig_t, activated)},
	{NULL},
    };
    const struct json_attr_t json_attrs_devices[] = {
	{"class", t_check,.dflt.check = "DEVICES"},
	{"devices", t_array, STRUCTARRAY(devicelist.list,
					 json_attrs_subdevice,
					 &devicelist.ndevices)},
	{NULL},
    };
    int status;

    memset(&devicelist, '\0', sizeof(devicelist));
    status = json_read_object(buf, json_attrs_devices, NULL);
    if (status != 0) {
	return status;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int i, status = 0;

    status = json_devicelist_read(argv[1]);
    printf("%d devices:\n", devicelist.ndevices);
    for (i = 0; i < devicelist.ndevices; i++)
	printf("%s @ %f\n", 
	       devicelist.list[i].path, devicelist.list[i].activated);

    if (status != 0)
	puts(json_error_string(status));
}

/* end */
