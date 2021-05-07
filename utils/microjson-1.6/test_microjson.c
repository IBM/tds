/* test_mjson.c - unit test for JSON parsing into fixed-extent structures
 *
 * This file is Copyright (c) 2014 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <stddef.h>

#include "mjson.h"

/*
 * Many of these structures and examples were dissected out of the GPSD code.
 */

#define MAXCHANNELS	20
#define MAXUSERDEVS	4
#define JSON_DATE_MAX	24	/* ISO8601 timestamp with 2 decimal places */

/* these values don't matter in themselves, they just have to be out-of-band */
#define DEVDEFAULT_BPS  	0
#define DEVDEFAULT_PARITY	'X'
#define DEVDEFAULT_STOPBITS	3
#define DEVDEFAULT_NATIVE	-1

typedef double timestamp_t;	/* Unix time in seconds with fractional part */

struct dop_t {
    /* Dilution of precision factors */
    double xdop, ydop, pdop, hdop, vdop, tdop, gdop;
};

struct version_t {
    char release[64];			/* external version */
    char rev[64];			/* internal revision ID */
    int proto_major, proto_minor;	/* API major and minor versions */
    char remote[PATH_MAX];		/* could be from a remote device */
};

struct devconfig_t {
    char path[PATH_MAX];
    int flags;
#define SEEN_GPS 	0x01
#define SEEN_RTCM2	0x02
#define SEEN_RTCM3	0x04
#define SEEN_AIS 	0x08
    char driver[64];
    char subtype[64];
    double activated;
    unsigned int baudrate, stopbits;	/* RS232 link parameters */
    char parity;			/* 'N', 'O', or 'E' */
    double cycle, mincycle;     	/* refresh cycle time in seconds */
    int driver_mode;    		/* is driver in native mode or not? */
};
struct gps_fix_t {
    timestamp_t time;	/* Time of update */
    int    mode;	/* Mode of fix */
#define MODE_NOT_SEEN	0	/* mode update not seen yet */
#define MODE_NO_FIX	1	/* none */
#define MODE_2D  	2	/* good for latitude/longitude */
#define MODE_3D  	3	/* good for altitude/climb too */
    double ept;		/* Expected time uncertainty */
    double latitude;	/* Latitude in degrees (valid if mode >= 2) */
    double epy;  	/* Latitude position uncertainty, meters */
    double longitude;	/* Longitude in degrees (valid if mode >= 2) */
    double epx;  	/* Longitude position uncertainty, meters */
    double altitude;	/* Altitude in meters (valid if mode == 3) */
    double epv;  	/* Vertical position uncertainty, meters */
    double track;	/* Course made good (relative to true north) */
    double epd;		/* Track uncertainty, degrees */
    double speed;	/* Speed over ground, meters/sec */
    double eps;		/* Speed uncertainty, meters/sec */
    double climb;       /* Vertical speed, meters/sec */
    double epc;		/* Vertical speed uncertainty */
};

struct gps_data_t {
    struct gps_fix_t	fix;	/* accumulated PVT data */

    /* this should move to the per-driver structure */
    double separation;		/* Geoidal separation, MSL - WGS84 (Meters) */

    /* GPS status -- always valid */
    int    status;		/* Do we have a fix? */
#define STATUS_NO_FIX	0	/* no */
#define STATUS_FIX	1	/* yes, without DGPS */
#define STATUS_DGPS_FIX	2	/* yes, with DGPS */

    /* precision of fix -- valid if satellites_used > 0 */
    int satellites_used;	/* Number of satellites used in solution */
    int used[MAXCHANNELS];	/* PRNs of satellites used in solution */
    struct dop_t dop;

    /* redundant with the estimate elements in the fix structure */
    double epe;  /* spherical position error, 95% confidence (meters)  */

    /* satellite status -- valid when satellites_visible > 0 */
    timestamp_t skyview_time;	/* skyview timestamp */
    int satellites_visible;	/* # of satellites in view */
    int PRN[MAXCHANNELS];	/* PRNs of satellite */
    int elevation[MAXCHANNELS];	/* elevation of satellite */
    int azimuth[MAXCHANNELS];	/* azimuth */
    double ss[MAXCHANNELS];	/* signal-to-noise ratio (dB) */

    struct devconfig_t dev;	/* device that shipped last update */

    struct {
	timestamp_t time;
	int ndevices;
	struct devconfig_t list[MAXUSERDEVS];
    } devices;

    struct version_t version;
};

static struct gps_data_t gpsdata;

static int json_tpv_read(const char *buf, struct gps_data_t *gpsdata,
			 const char **endptr)
{
    const struct json_attr_t json_attrs_1[] = {
	/* *INDENT-OFF* */
	{"class",  t_check,   .dflt.check = "TPV"},
	{"device", t_string,  .addr.string = gpsdata->dev.path,
			         .len = sizeof(gpsdata->dev.path)},
#ifdef TIME_ENABLE
	{"time",   t_time,    .addr.real = &gpsdata->fix.time,
			         .dflt.real = NAN},
#else
	{"time",   t_ignore},
#endif /* TIME_ENABLE */
	{"ept",    t_real,    .addr.real = &gpsdata->fix.ept,
			         .dflt.real = NAN},
	{"lon",    t_real,    .addr.real = &gpsdata->fix.longitude,
			         .dflt.real = NAN},
	{"lat",    t_real,    .addr.real = &gpsdata->fix.latitude,
			         .dflt.real = NAN},
	{"alt",    t_real,    .addr.real = &gpsdata->fix.altitude,
			         .dflt.real = NAN},
	{"epx",    t_real,    .addr.real = &gpsdata->fix.epx,
			         .dflt.real = NAN},
	{"epy",    t_real,    .addr.real = &gpsdata->fix.epy,
			         .dflt.real = NAN},
	{"epv",    t_real,    .addr.real = &gpsdata->fix.epv,
			         .dflt.real = NAN},
	{"track",   t_real,   .addr.real = &gpsdata->fix.track,
			         .dflt.real = NAN},
	{"speed",   t_real,   .addr.real = &gpsdata->fix.speed,
			         .dflt.real = NAN},
	{"climb",   t_real,   .addr.real = &gpsdata->fix.climb,
			         .dflt.real = NAN},
	{"epd",    t_real,    .addr.real = &gpsdata->fix.epd,
			         .dflt.real = NAN},
	{"eps",    t_real,    .addr.real = &gpsdata->fix.eps,
			         .dflt.real = NAN},
	{"epc",    t_real,    .addr.real = &gpsdata->fix.epc,
			         .dflt.real = NAN},
	{"mode",   t_integer, .addr.integer = &gpsdata->fix.mode,
			         .dflt.integer = MODE_NOT_SEEN},
	{NULL},
	/* *INDENT-ON* */
    };

    return json_read_object(buf, json_attrs_1, endptr);
}

static int json_sky_read(const char *buf, struct gps_data_t *gpsdata,
			 const char **endptr)
{
    bool usedflags[MAXCHANNELS];
    const struct json_attr_t json_attrs_2_1[] = {
	/* *INDENT-OFF* */
	{"PRN",	   t_integer, .addr.integer = gpsdata->PRN},
	{"el",	   t_integer, .addr.integer = gpsdata->elevation},
	{"az",	   t_integer, .addr.integer = gpsdata->azimuth},
	{"ss",	   t_real,    .addr.real = gpsdata->ss},
	{"used",   t_boolean, .addr.boolean = usedflags},
	/* *INDENT-ON* */
	{NULL},
    };
    const struct json_attr_t json_attrs_2[] = {
	/* *INDENT-OFF* */
	{"class",      t_check,   .dflt.check = "SKY"},
	{"device",     t_string,  .addr.string  = gpsdata->dev.path,
	                             .len = sizeof(gpsdata->dev.path)},
	{"hdop",       t_real,    .addr.real    = &gpsdata->dop.hdop,
	                             .dflt.real = NAN},
	{"xdop",       t_real,    .addr.real    = &gpsdata->dop.xdop,
	                             .dflt.real = NAN},
	{"ydop",       t_real,    .addr.real    = &gpsdata->dop.ydop,
	                             .dflt.real = NAN},
	{"vdop",       t_real,    .addr.real    = &gpsdata->dop.vdop,
	                             .dflt.real = NAN},
	{"tdop",       t_real,    .addr.real    = &gpsdata->dop.tdop,
	                             .dflt.real = NAN},
	{"pdop",       t_real,    .addr.real    = &gpsdata->dop.pdop,
	                             .dflt.real = NAN},
	{"gdop",       t_real,    .addr.real    = &gpsdata->dop.gdop,
	                             .dflt.real = NAN},
	{"satellites", t_array,   .addr.array.element_type = t_object,
				     .addr.array.arr.objects.subtype=json_attrs_2_1,
	                             .addr.array.maxlen = MAXCHANNELS,
	                             .addr.array.count = &gpsdata->satellites_visible},
	{NULL},
	/* *INDENT-ON* */
    };
    int status, i, j;

    for (i = 0; i < MAXCHANNELS; i++) {
	gpsdata->PRN[i] = 0;
	usedflags[i] = false;
    }

    status = json_read_object(buf, json_attrs_2, endptr);
    if (status != 0)
	return status;

    gpsdata->satellites_used = 0;
    gpsdata->satellites_visible = 0;
    (void)memset(gpsdata->used, '\0', sizeof(gpsdata->used));
    for (i = j = 0; i < MAXCHANNELS; i++) {
	if(gpsdata->PRN[i] > 0)
	    gpsdata->satellites_visible++;
	if (usedflags[i]) {
	    gpsdata->used[j++] = gpsdata->PRN[i];
	    gpsdata->satellites_used++;
	}
    }

    return 0;
}

static int json_devicelist_read(const char *buf, struct gps_data_t *gpsdata,
				const char **endptr)
{
    const struct json_attr_t json_attrs_subdevices[] = {
	/* *INDENT-OFF* */
	{"class",      t_check,      .dflt.check = "DEVICE"},
	{"path",       t_string,     STRUCTOBJECT(struct devconfig_t, path),
	                                .len = sizeof(gpsdata->devices.list[0].path)},
	{"activated",  t_real,       STRUCTOBJECT(struct devconfig_t, activated)},
	{"flags",      t_integer,    STRUCTOBJECT(struct devconfig_t, flags)},
	{"driver",     t_string,     STRUCTOBJECT(struct devconfig_t, driver),
	                                .len = sizeof(gpsdata->devices.list[0].driver)},
	{"subtype",    t_string,     STRUCTOBJECT(struct devconfig_t, subtype),
	                                .len = sizeof(gpsdata->devices.list[0].subtype)},
	{"native",     t_integer,    STRUCTOBJECT(struct devconfig_t, driver_mode),
				        .dflt.integer = -1},
	{"bps",	       t_uinteger,   STRUCTOBJECT(struct devconfig_t, baudrate),
				        .dflt.uinteger = DEVDEFAULT_BPS},
	{"parity",     t_character,  STRUCTOBJECT(struct devconfig_t, parity),
	                                .dflt.character = DEVDEFAULT_PARITY},
	{"stopbits",   t_uinteger,   STRUCTOBJECT(struct devconfig_t, stopbits),
				        .dflt.integer = DEVDEFAULT_STOPBITS},
	{"cycle",      t_real,       STRUCTOBJECT(struct devconfig_t, cycle),
				        .dflt.real = NAN},
	{"mincycle",   t_real,       STRUCTOBJECT(struct devconfig_t, mincycle),
				        .dflt.real = NAN},
	{NULL},
	/* *INDENT-ON* */
    };
    const struct json_attr_t json_attrs_devices[] = {
	{"class", t_check,.dflt.check = "DEVICES"},
	{"devices", t_array, STRUCTARRAY(gpsdata->devices.list,
					 json_attrs_subdevices,
					 &gpsdata->devices.ndevices)},
	{NULL},
    };
    int status;

    memset(&gpsdata->devices, '\0', sizeof(gpsdata->devices));
    status = json_read_object(buf, json_attrs_devices, endptr);
    if (status != 0) {
	return status;
    }
    return 0;
}

static int json_device_read(const char *buf,
		     struct devconfig_t *dev,
		     const char **endptr)
{
    char tbuf[JSON_DATE_MAX+1];
    const struct json_attr_t json_attrs_device[] = {
	{"class",      t_check,      .dflt.check = "DEVICE"},

        {"path",       t_string,     .addr.string  = dev->path,
	                                .len = sizeof(dev->path)},
	{"activated",  t_string,     .addr.string = tbuf,
			                .len = sizeof(tbuf)},
	{"activated",  t_real,       .addr.real = &dev->activated},
	{"flags",      t_integer,    .addr.integer = &dev->flags},
	{"driver",     t_string,     .addr.string  = dev->driver,
	                                .len = sizeof(dev->driver)},
	{"subtype",    t_string,     .addr.string  = dev->subtype,
	                                .len = sizeof(dev->subtype)},
	{"native",     t_integer,    .addr.integer = &dev->driver_mode,
				        .dflt.integer = DEVDEFAULT_NATIVE},
	{"bps",	       t_uinteger,   .addr.uinteger = &dev->baudrate,
				        .dflt.uinteger = DEVDEFAULT_BPS},
	{"parity",     t_character,  .addr.character = &dev->parity,
                                        .dflt.character = DEVDEFAULT_PARITY},
	{"stopbits",   t_uinteger,   .addr.uinteger = &dev->stopbits,
				        .dflt.uinteger = DEVDEFAULT_STOPBITS},
	{"cycle",      t_real,       .addr.real = &dev->cycle,
				        .dflt.real = NAN},
	{"mincycle",   t_real,       .addr.real = &dev->mincycle,
				        .dflt.real = NAN},
	{NULL},
    };
    /* *INDENT-ON* */
    int status;

    tbuf[0] = '\0';
    status = json_read_object(buf, json_attrs_device, endptr);
    if (status != 0)
	return status;

    return 0;
}

static int json_version_read(const char *buf, struct gps_data_t *gpsdata,
			     const char **endptr)
{
    const struct json_attr_t json_attrs_version[] = {
	/* *INDENT-OFF* */
        {"class",     t_check,   .dflt.check = "VERSION"},
	{"release",   t_string,  .addr.string  = gpsdata->version.release,
	                            .len = sizeof(gpsdata->version.release)},
	{"rev",       t_string,  .addr.string  = gpsdata->version.rev,
	                            .len = sizeof(gpsdata->version.rev)},
	{"proto_major", t_integer, .addr.integer = &gpsdata->version.proto_major},
	{"proto_minor", t_integer, .addr.integer = &gpsdata->version.proto_minor},
	{"remote",    t_string,  .addr.string  = gpsdata->version.remote,
	                            .len = sizeof(gpsdata->version.remote)},
	{NULL},
	/* *INDENT-ON* */
    };
    int status;

    memset(&gpsdata->version, '\0', sizeof(gpsdata->version));
    status = json_read_object(buf, json_attrs_version, endptr);

    return status;
}

static int libgps_json_unpack(const char *buf,
		       struct gps_data_t *gpsdata, const char **end)
/* the only entry point - unpack a JSON object into gpsdata_t substructures */
{
    int status;
    char *classtag = strstr(buf, "\"class\":");

    if (classtag == NULL)
	return -1;
#define STARTSWITH(str, prefix)	strncmp(str, prefix, sizeof(prefix)-1)==0
    if (STARTSWITH(classtag, "\"class\":\"TPV\"")) {
	status = json_tpv_read(buf, gpsdata, end);
	gpsdata->status = STATUS_FIX;
	return status;
    } else if (STARTSWITH(classtag, "\"class\":\"SKY\"")) {
	status = json_sky_read(buf, gpsdata, end);
	return status;
    } else if (STARTSWITH(classtag, "\"class\":\"DEVICE\"")) {
	status = json_device_read(buf, &gpsdata->dev, end);
	return status;
    } else if (STARTSWITH(classtag, "\"class\":\"DEVICES\"")) {
	status = json_devicelist_read(buf, gpsdata, end);
	return status;
    } else if (STARTSWITH(classtag, "\"class\":\"VERSION\"")) {
	status = json_version_read(buf, gpsdata, end);
	return status;
    }
#undef STARTSWITH
}

static int assert_error_case(int num, int status, int error)
{
    if (status != error) {
	(void)fprintf(stderr, "case %d FAILED, status %d (%s).\n", num,
		      status, json_error_string(status));
	exit(EXIT_FAILURE);
    }
    else
	return 0;
}

static void assert_case(int num, int status)
{
    assert_error_case(num, status, 0);
}

static void assert_string(char *attr, char *fld, char *check)
{
    if (strcmp(fld, check)) {
	(void)fprintf(stderr,
		      "'%s' expecting string '%s', got '%s'.\n",
		      attr, check, fld);
	exit(EXIT_FAILURE);
    }
}

static void assert_integer(char *attr, int fld, int check)
{
    if (fld != check) {
	(void)fprintf(stderr,
		      "'%s' expecting integer %d, got %d.\n",
		      attr, check, fld);
	exit(EXIT_FAILURE);
    }
}

static void assert_uinteger(char *attr, unsigned int fld, unsigned int check)
{
    if (fld != check) {
	(void)fprintf(stderr,
		      "'%s' expecting uinteger %u, got %u.\n",
		      attr, check, fld);
	exit(EXIT_FAILURE);
    }
}

static void assert_boolean(char *attr, bool fld, bool check)
{
    if (fld != check) {
	(void)fprintf(stderr,
		      "'%s' expecting boolean %s, got %s.\n",
		      attr, 
		      check ? "true" : "false",
		      fld ? "true" : "false");
	exit(EXIT_FAILURE);
    }
}

/*
 * Floating point comparisons are iffy, but at least if any of these fail
 * the output will make it clear whether it was a precision issue
 */
static void assert_real(char *attr, double fld, double check)
{
    if (fld != check) {
	(void)fprintf(stderr,
		      "'%s' expecting real %f got %f.\n", 
		      attr, check, fld);
	exit(EXIT_FAILURE);
    }
}

/* Case 1: TPV report */

/* *INDENT-OFF* */
static const char json_str1[] = "{\"class\":\"TPV\",\
    \"device\":\"GPS#1\",				\
    \"time\":\"2005-06-19T12:12:42.03Z\",		\
    \"lon\":46.498203637,\"lat\":7.568074350,           \
    \"alt\":1327.780,\"epx\":21.000,\"epy\":23.000,\"epv\":124.484,\"mode\":3}";

/* Case 2: SKY report */

static const char *json_str2 = "{\"class\":\"SKY\",\
         \"satellites\":[\
         {\"PRN\":10,\"el\":45,\"az\":196,\"ss\":34,\"used\":true},\
         {\"PRN\":29,\"el\":67,\"az\":310,\"ss\":40,\"used\":true},\
         {\"PRN\":28,\"el\":59,\"az\":108,\"ss\":42,\"used\":true},\
         {\"PRN\":26,\"el\":51,\"az\":304,\"ss\":43,\"used\":true},\
         {\"PRN\":8,\"el\":44,\"az\":58,\"ss\":41,\"used\":true},\
         {\"PRN\":27,\"el\":16,\"az\":66,\"ss\":39,\"used\":true},\
         {\"PRN\":21,\"el\":10,\"az\":301,\"ss\":0,\"used\":false}]}";

/* Case 3: String list syntax */

static const char *json_str3 = "[\"foo\",\"bar\",\"baz\"]";

static char *stringptrs[3];
static char stringstore[256];
static int stringcount;

static const struct json_array_t json_array_3 = {
    .element_type = t_string,
    .arr.strings.ptrs = stringptrs,
    .arr.strings.store = stringstore,
    .arr.strings.storelen = sizeof(stringstore),
    .count = &stringcount,
    .maxlen = sizeof(stringptrs)/sizeof(stringptrs[0]),
};

/* Case 4: test defaulting of unspecified attributes */

static const char *json_str4 = "{\"flag1\":true,\"flag2\":false}";

static bool flag1, flag2;
static double dftreal;
static int dftinteger;
static unsigned int dftuinteger;

static const struct json_attr_t json_attrs_4[] = {
    {"dftint",  t_integer, .addr.integer = &dftinteger, .dflt.integer = -5},
    {"dftuint", t_integer, .addr.uinteger = &dftuinteger, .dflt.uinteger = 10},
    {"dftreal", t_real,    .addr.real = &dftreal,       .dflt.real = 23.17},
    {"flag1",   t_boolean, .addr.boolean = &flag1,},
    {"flag2",   t_boolean, .addr.boolean = &flag2,},
    {NULL},
};

/* Case 5: test DEVICE parsing */

static const char *json_str5 = "{\"class\":\"DEVICE\",\
           \"path\":\"/dev/ttyUSB0\",\
           \"flags\":5,\
           \"driver\":\"Foonly\",\"subtype\":\"Foonly Frob\"\
           }";

/* Case 6: test parsing of subobject list into array of structures */

static const char *json_str6 = "{\"parts\":[\
           {\"name\":\"Urgle\", \"flag\":true, \"count\":3},\
           {\"name\":\"Burgle\",\"flag\":false,\"count\":1},\
           {\"name\":\"Witter\",\"flag\":true, \"count\":4},\
           {\"name\":\"Thud\",  \"flag\":false,\"count\":1}]}";

struct dumbstruct_t {
    char name[64];
    bool flag;
    int count;
};
static struct dumbstruct_t dumbstruck[5];
static int dumbcount;

static const struct json_attr_t json_attrs_6_subtype[] = {
    {"name",  t_string,  .addr.offset = offsetof(struct dumbstruct_t, name),
                         .len = 64},
    {"flag",  t_boolean, .addr.offset = offsetof(struct dumbstruct_t, flag),},
    {"count", t_integer, .addr.offset = offsetof(struct dumbstruct_t, count),},
    {NULL},
};

static const struct json_attr_t json_attrs_6[] = {
    {"parts", t_array, .addr.array.element_type = t_structobject,
                       .addr.array.arr.objects.base = (char*)&dumbstruck,
                       .addr.array.arr.objects.stride = sizeof(struct dumbstruct_t),
                       .addr.array.arr.objects.subtype = json_attrs_6_subtype,
                       .addr.array.count = &dumbcount,
                       .addr.array.maxlen = sizeof(dumbstruck)/sizeof(dumbstruck[0])},
    {NULL},
};

/* Case 7: test parsing of version response */

static const char *json_str7 = "{\"class\":\"VERSION\",\
           \"release\":\"2.40dev\",\"rev\":\"dummy-revision\",\
           \"proto_major\":3,\"proto_minor\":1}";

/* Case 8: test parsing arrays of enumerated types */

static const char *json_str8 = "{\"fee\":\"FOO\",\"fie\":\"BAR\",\"foe\":\"BAZ\"}";
static const struct json_enum_t enum_table[] = {
    {"BAR", 6}, {"FOO", 3}, {"BAZ", 14}, {NULL}
};

static int fee, fie, foe;
static const struct json_attr_t json_attrs_8[] = {
    {"fee",  t_integer, .addr.integer = &fee, .map=enum_table},
    {"fie",  t_integer, .addr.integer = &fie, .map=enum_table},
    {"foe",  t_integer, .addr.integer = &foe, .map=enum_table},
    {NULL},
};

/* Case 9: Like case 6 but w/ an empty array */

static const char *json_str9 = "{\"parts\":[]}";

/* Case 10: test buffer overflow of short string destination */

// static char json_strErr2[7 * JSON_VAL_MAX];  /* dynamically built */
static char *json_strOver = "{\"name\":\"\\u0033\\u0034\\u0035\\u0036\"}";

char json_short_string_dst[1];
static const struct json_attr_t json_short_string[] = {
    {"name", t_string,
        .addr.string = json_short_string_dst,
        .len = sizeof(json_short_string_dst)},
    {NULL},
};
 
/* Case 11: Read array of integers */

static const char *json_str11 = "[23,-17,5]";
static int intstore[4], intcount;

static const struct json_array_t json_array_11 = {
    .element_type = t_integer,
    .arr.integers.store = intstore,
    .count = &intcount,
    .maxlen = sizeof(intstore)/sizeof(intstore[0]),
};

/* Case 12: Read array of booleans */

static const char *json_str12 = "[true,false,true]";
static bool boolstore[4];
static int boolcount;

static const struct json_array_t json_array_12 = {
    .element_type = t_boolean,
    .arr.booleans.store = boolstore,
    .count = &boolcount,
    .maxlen = sizeof(boolstore)/sizeof(boolstore[0]),
};

/* Case 13: Read array of reals */

static const char *json_str13 = "[23.1,-17.2,5.3]";
static double realstore[4]; 
static int realcount;

static const struct json_array_t json_array_13 = {
    .element_type = t_real,
    .arr.reals.store = realstore,
    .count = &realcount,
    .maxlen = sizeof(realstore)/sizeof(realstore[0]),
};

/* Case 14: Read object within object. Test case not derived from GPSD */

static char json_inner_name_string_dst[JSON_VAL_MAX];
static int inner_value;
static const char *json_str14 = "{\"name\":\"wobble\",\"value\":{\"inner\":23}}";
const struct json_attr_t json_inner_int_value[] = {
    {"inner", t_integer,
         .addr.integer = &inner_value},
    {NULL},
};
static const struct json_attr_t json_object_14[] = {
    {"name", t_string,
        .addr.string = json_inner_name_string_dst,
        .len = sizeof(json_inner_name_string_dst)},
    {"value", t_object,
         .addr.attrs = json_inner_int_value},
    {NULL},
};

/* Case 15: test parsing 0/1 integer values as bool */

static bool flag3;
static bool flags4[3];

static const char *json_str15 = "{\"flag1\":1, \"flag2\":0, \"flag3\":7, \"flags4\":[1,0,7]}";

static const struct json_attr_t json_attrs_15[] = {
    {"flag1",       t_boolean,	.addr.boolean = &flag1,},
    {"flag2",       t_boolean,	.addr.boolean = &flag2,},
    {"flag3",       t_boolean,	.addr.boolean = &flag3,},
    {"flags4",      t_array,	.addr.array.element_type = t_boolean,
				.addr.array.arr.booleans = flags4,
				.addr.array.maxlen = 3,},
    {NULL},
};

/* Case 16: Read object within object within object.*/

char json_inner1_name_string_dst[JSON_VAL_MAX];

int inner1_value;
int inner2_value;

static const char *json_str16 = "{\"name\":\"wobble\",\"value\":{\"inner\":23, \"innerobject\":{\"innerinner\":123}}}\0READ PAST END OF BUFFER";

const struct json_attr_t json_inner2_int_value[] = {
	{"innerinner", t_integer,
		 .addr.integer = &inner2_value},
	{NULL},
};
const struct json_attr_t json_inner1_int_value[] = {
	{"inner", t_integer,
		 .addr.integer = &inner1_value},
	{"innerobject", t_object,
		 .addr.attrs = json_inner2_int_value},
	{NULL},
};
static const struct json_attr_t json_object_16[] = {
	{"name", t_string,
		.addr.string = json_inner1_name_string_dst,
		.len = sizeof(json_inner1_name_string_dst)},
	{"value", t_object,
		 .addr.attrs = json_inner1_int_value},
	{NULL},
};

/* Case 17: Parent struct enclosing enum in sub-struct followed by value outside of sub-struct */

static int eval, ival;
static const char *json_str17 = "{\"parentStruct\":{\"enumStruct\":{\"enumName\":\"NOT_SET\"\n},\"intName\":1}}";
static const struct json_enum_t enum_table16[] = {
    {"NOT_SET", 0}, {"SET", 1},{NULL}
};

static const struct json_attr_t json_enum_struct[] = {
	{"enumName",  t_integer,.addr.integer = &eval,.map = enum_table16},
	{NULL},
};
static const struct json_attr_t json_sub_struct[] = {
	{"enumStruct",  t_object,.addr.attrs = json_enum_struct},
	{"intName",  t_integer,.addr.integer = &ival},
	{NULL},
};
static const struct json_attr_t json_object_17[] = {
	{"parentStruct",  t_object,.addr.attrs = json_sub_struct},
	{NULL},
};

/* Case 18: Call json_read_object() on concatenated JSON objects. */

static const char *json_cur18;
static const char *json_end18;

static const char *json_str18 = "{\"flag1\":1}{\"flag1\":0}{\"flag1\":7, \"flags4\":[1,0,7]} {\"flag2\":true, \"flags4\":[0,true,false]}";

/* Insert more test definitions here */
/* *INDENT-ON* */

static void jsontest(int i)
{
    int status = 0;

    switch (i) 
    {
    case 1:
	status = libgps_json_unpack(json_str1, &gpsdata, NULL);
	assert_case(i, status);
	assert_string("device", gpsdata.dev.path, "GPS#1");
#ifdef TIME_ENABLE
	assert_real("time", gpsdata.fix.time, 1119183162.030000);
#endif /* TIME_ENABLE */
	assert_integer("mode", gpsdata.fix.mode, 3);
	assert_real("lon", gpsdata.fix.longitude, 46.498203637);
	assert_real("lat", gpsdata.fix.latitude, 7.568074350);
	break;

    case 2:
	status = libgps_json_unpack(json_str2, &gpsdata, NULL);
	assert_case(i, status);
	assert_integer("used", gpsdata.satellites_used, 6);
	assert_integer("PRN[0]", gpsdata.PRN[0], 10);
	assert_integer("el[0]", gpsdata.elevation[0], 45);
	assert_integer("az[0]", gpsdata.azimuth[0], 196);
	assert_real("ss[0]", gpsdata.ss[0], 34);
	assert_integer("used[0]", gpsdata.used[0], 10);
	assert_integer("used[5]", gpsdata.used[5], 27);
	assert_integer("PRN[6]", gpsdata.PRN[6], 21);
	assert_integer("el[6]", gpsdata.elevation[6], 10);
	assert_integer("az[6]", gpsdata.azimuth[6], 301);
	assert_real("ss[6]", gpsdata.ss[6], 0);
	break;

    case 3:
	status = json_read_array(json_str3, &json_array_3, NULL);
	assert_case(i, status);
	assert(stringcount == 3);
	assert(strcmp(stringptrs[0], "foo") == 0);
	assert(strcmp(stringptrs[1], "bar") == 0);
	assert(strcmp(stringptrs[2], "baz") == 0);
	break;

    case 4:
	status = json_read_object(json_str4, json_attrs_4, NULL);
	assert_case(i, status);
	assert_integer("dftint", dftinteger, -5);	/* did the default work? */
	assert_uinteger("dftuint", dftuinteger, 10);	/* did the default work? */
	assert_real("dftreal", dftreal, 23.17);	/* did the default work? */
	assert_boolean("flag1", flag1, true);
	assert_boolean("flag2", flag2, false);
	break;

    case 5:
	status = libgps_json_unpack(json_str5, &gpsdata, NULL);
	assert_case(i, status);
	assert_string("path", gpsdata.dev.path, "/dev/ttyUSB0");
	assert_integer("flags", gpsdata.dev.flags, 5);
	assert_string("driver", gpsdata.dev.driver, "Foonly");
	break;

    case 6:
	status = json_read_object(json_str6, json_attrs_6, NULL);
	assert_case(i, status);
	assert_integer("dumbcount", dumbcount, 4);
	assert_string("dumbstruck[0].name", dumbstruck[0].name, "Urgle");
	assert_string("dumbstruck[1].name", dumbstruck[1].name, "Burgle");
	assert_string("dumbstruck[2].name", dumbstruck[2].name, "Witter");
	assert_string("dumbstruck[3].name", dumbstruck[3].name, "Thud");
	assert_boolean("dumbstruck[0].flag", dumbstruck[0].flag, true);
	assert_boolean("dumbstruck[1].flag", dumbstruck[1].flag, false);
	assert_boolean("dumbstruck[2].flag", dumbstruck[2].flag, true);
	assert_boolean("dumbstruck[3].flag", dumbstruck[3].flag, false);
	assert_integer("dumbstruck[0].count", dumbstruck[0].count, 3);
	assert_integer("dumbstruck[1].count", dumbstruck[1].count, 1);
	assert_integer("dumbstruck[2].count", dumbstruck[2].count, 4);
	assert_integer("dumbstruck[3].count", dumbstruck[3].count, 1);
	break;

    case 7:
	status = libgps_json_unpack(json_str7, &gpsdata, NULL);
	assert_case(i, status);
	assert_string("release", gpsdata.version.release, "2.40dev");
	assert_string("rev", gpsdata.version.rev, "dummy-revision");
	assert_integer("proto_major", gpsdata.version.proto_major, 3);
	assert_integer("proto_minor", gpsdata.version.proto_minor, 1);
	break;

    case 8:
	status = json_read_object(json_str8, json_attrs_8, NULL);
	assert_case(i, status);
	assert_integer("fee", fee, 3);
	assert_integer("fie", fie, 6);
	assert_integer("foe", foe, 14);
	break;

    case 9:
	/* yes, the '6' in the next line is correct */ 
	status = json_read_object(json_str9, json_attrs_6, NULL);
	assert_case(i, status);
	assert_integer("dumbcount", dumbcount, 0);
	break;

    case 10:
	status = json_read_object(json_strOver, json_short_string, NULL);
	status = assert_error_case(10, status, JSON_ERR_STRLONG);
	assert_string("name", json_short_string_dst, "");
 	break;

    case 11:
	status = json_read_array(json_str11, &json_array_11, NULL);
	assert_integer("count", intcount, 3);
	assert_integer("intstore[0]", intstore[0], 23);
	assert_integer("intstore[1]", intstore[1], -17);
	assert_integer("intstore[2]", intstore[2], 5);
	assert_integer("intstore[3]", intstore[3], 0);
	break;

    case 12:
	status = json_read_array(json_str12, &json_array_12, NULL);
	assert_integer("count", boolcount, 3);
	assert_boolean("boolstore[0]", boolstore[0], true);
	assert_boolean("boolstore[1]", boolstore[1], false);
	assert_boolean("boolstore[2]", boolstore[2], true);
	assert_boolean("boolstore[3]", boolstore[3], false);
	break;

    case 13:
	status = json_read_array(json_str13, &json_array_13, NULL);
	assert_integer("count", realcount, 3);
	assert_real("realstore[0]", realstore[0], 23.1);
	assert_real("realstore[1]", realstore[1], -17.2);
	assert_real("realstore[2]", realstore[2], 5.3);
	assert_real("realstore[3]", realstore[3], 0);
	break;

    case 14:
	status = json_read_object(json_str14, json_object_14, NULL);
	assert_case(i, status);
	assert_integer("inner", inner_value, 23);
	assert_string("name", json_inner_name_string_dst, "wobble");
	break;

    case 15:
	status = json_read_object(json_str15, json_attrs_15, NULL);
	assert_case(i, status);
	assert_boolean("flag1", flag1, true);
	assert_boolean("flag2", flag2, false);
	assert_boolean("flag3", flag3, true);
	assert_boolean("flags4[0]", flags4[0], true);
	assert_boolean("flags4[1]", flags4[1], false);
	assert_boolean("flags4[2]", flags4[2], true);
	break;
	
    case 16:
	status = json_read_object(json_str16, json_object_16, NULL);
	assert_case(i, status);
	assert_integer("inner", inner1_value, 23);
	assert_integer("innerinner", inner2_value, 123);
	assert_string("name", json_inner1_name_string_dst, "wobble");
	break;

    case 17:
	status = json_read_object(json_str17, json_object_17, NULL);
	assert_case(i, status);
	assert_integer("ival", ival, 1);
	assert_integer("eval", eval, 0);
	break;

    case 18:
	json_cur18 = json_str18;
	json_end18 = json_str18 + strlen(json_str18);
	/* first JSON message */
	status = json_read_object(json_cur18, json_attrs_15, &json_cur18);
	assert_case(i, status);
	assert_boolean("flag1", flag1, true);
	/* second JSON message */
	status = json_read_object(json_cur18, json_attrs_15, &json_cur18);
	assert_case(i, status);
	assert_boolean("flag1", flag1, false);
	/* third JSON message */
	status = json_read_object(json_cur18, json_attrs_15, &json_cur18);
	assert_case(i, status);
	assert_boolean("flag1", flag1, true);
	assert_boolean("flags4[0]", flags4[0], true);
	assert_boolean("flags4[1]", flags4[1], false);
	assert_boolean("flags4[2]", flags4[2], true);
	/* fourth JSON message */
	status = json_read_object(json_cur18, json_attrs_15, &json_cur18);
	assert_case(i, status);
	assert_boolean("flag2", flag2, true);
	assert_boolean("flags4[0]", flags4[0], false);
	assert_boolean("flags4[1]", flags4[1], true);
	assert_boolean("flags4[2]", flags4[2], false);
	break;

#define MAXTEST 18

    default:
	(void)fputs("Unknown test number\n", stderr);
	break;
    }

    if (status > 0)
	printf("Parse failure!\n");
}

int main(int argc, char *argv[])
{
    int option;
    int individual = 0;

    while ((option = getopt(argc, argv, "hn:D:?")) != -1) {
	switch (option) {
	case 'D':
#ifdef DEBUG_ENABLE
	    json_enable_debug(atoi(optarg), stdout);
#else
	    fputs("Debug disabled in build\n", stderr);
#endif
	    break;
	case 'n':
	    individual = atoi(optarg);
	    break;
	case '?':
	case 'h':
	default:
	    (void)fputs("usage: test_json [-D lvl]\n", stderr);
	    exit(EXIT_FAILURE);
	}
    }

    (void)fprintf(stderr, "microjson unit test ");

    if (individual)
	jsontest(individual);
    else {
	int i;
	for (i = 1; i <= MAXTEST; i++)
	    jsontest(i);
    }

    (void)fprintf(stderr, "succeeded.\n");

    exit(EXIT_SUCCESS);
}

/* end */
