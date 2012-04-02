/* Time module */

#include "Python.h"

#include <ctype.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef QUICKWIN
#include <io.h>
#endif

#if defined(__WATCOMC__) && !defined(__QNX__)
#include <i86.h>
#else
#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "pythread.h"

#if defined(__BORLANDC__)
/* These overrides not needed for Win32 */
#define timezone _timezone
#define tzname _tzname
#define daylight _daylight
#endif /* __BORLANDC__ */
#endif /* MS_WINDOWS */
#endif /* !__WATCOMC__ || __QNX__ */

#if defined(PYOS_OS2)
#define INCL_DOS
#define INCL_ERRORS
#include <os2.h>
#endif

#if defined(PYCC_VACPP)
#include <sys/time.h>
#endif

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

/* Forward declarations */
static int floatsleep(double);
static PyObject* floattime(void);

static PyObject *
time_time(PyObject *self, PyObject *unused)
{
    return floattime();
}

PyDoc_STRVAR(time_doc,
"time() -> floating point number\n\
\n\
Return the current time in seconds since the Epoch.\n\
Fractions of a second may be present if the system clock provides them.");

#if defined(HAVE_CLOCK)

#ifndef CLOCKS_PER_SEC
#ifdef CLK_TCK
#define CLOCKS_PER_SEC CLK_TCK
#else
#define CLOCKS_PER_SEC 1000000
#endif
#endif

static PyObject *
pyclock(void)
{
    clock_t value;
    value = clock();
    if (value == (clock_t)-1) {
        PyErr_SetString(PyExc_RuntimeError,
                "the processor time used is not available "
                "or its value cannot be represented");
        return NULL;
    }
    return PyFloat_FromDouble((double)value / CLOCKS_PER_SEC);
}
#endif /* HAVE_CLOCK */

#if defined(MS_WINDOWS) && !defined(__BORLANDC__)
/* Win32 has better clock replacement; we have our own version, due to Mark
   Hammond and Tim Peters */
static PyObject *
win32_clock(int fallback)
{
    static LONGLONG cpu_frequency = 0;
    static LONGLONG ctrStart;
    LARGE_INTEGER now;
    double diff;

    if (cpu_frequency == 0) {
        LARGE_INTEGER freq;
        QueryPerformanceCounter(&now);
        ctrStart = now.QuadPart;
        if (!QueryPerformanceFrequency(&freq) || freq.QuadPart == 0) {
            /* Unlikely to happen - this works on all intel
               machines at least!  Revert to clock() */
            if (fallback)
                return pyclock();
            else
                return PyErr_SetFromWindowsErr(0);
        }
        cpu_frequency = freq.QuadPart;
    }
    QueryPerformanceCounter(&now);
    diff = (double)(now.QuadPart - ctrStart);
    return PyFloat_FromDouble(diff / (double)cpu_frequency);
}
#endif

#if (defined(MS_WINDOWS) && !defined(__BORLANDC__)) || defined(HAVE_CLOCK)
static PyObject *
time_clock(PyObject *self, PyObject *unused)
{
#if defined(MS_WINDOWS) && !defined(__BORLANDC__)
    return win32_clock(1);
#else
    return pyclock();
#endif
}

PyDoc_STRVAR(clock_doc,
"clock() -> floating point number\n\
\n\
Return the CPU time or real time since the start of the process or since\n\
the first call to clock().  This has as much precision as the system\n\
records.");
#endif

#ifdef HAVE_CLOCK_GETTIME
static PyObject *
time_clock_gettime(PyObject *self, PyObject *args)
{
    int ret;
    clockid_t clk_id;
    struct timespec tp;

    if (!PyArg_ParseTuple(args, "i:clock_gettime", &clk_id))
        return NULL;

    ret = clock_gettime((clockid_t)clk_id, &tp);
    if (ret != 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }

    return PyFloat_FromDouble(tp.tv_sec + tp.tv_nsec * 1e-9);
}

PyDoc_STRVAR(clock_gettime_doc,
"clock_gettime(clk_id) -> floating point number\n\
\n\
Return the time of the specified clock clk_id.");
#endif

#ifdef HAVE_CLOCK_GETRES
static PyObject *
time_clock_getres(PyObject *self, PyObject *args)
{
    int ret;
    clockid_t clk_id;
    struct timespec tp;

    if (!PyArg_ParseTuple(args, "i:clock_getres", &clk_id))
        return NULL;

    ret = clock_getres((clockid_t)clk_id, &tp);
    if (ret != 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }

    return PyFloat_FromDouble(tp.tv_sec + tp.tv_nsec * 1e-9);
}

PyDoc_STRVAR(clock_getres_doc,
"clock_getres(clk_id) -> floating point number\n\
\n\
Return the resolution (precision) of the specified clock clk_id.");
#endif

static PyObject *
time_sleep(PyObject *self, PyObject *args)
{
    double secs;
    if (!PyArg_ParseTuple(args, "d:sleep", &secs))
        return NULL;
    if (secs < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "sleep length must be non-negative");
        return NULL;
    }
    if (floatsleep(secs) != 0)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(sleep_doc,
"sleep(seconds)\n\
\n\
Delay execution for a given number of seconds.  The argument may be\n\
a floating point number for subsecond precision.");

static PyStructSequence_Field struct_time_type_fields[] = {
    {"tm_year", "year, for example, 1993"},
    {"tm_mon", "month of year, range [1, 12]"},
    {"tm_mday", "day of month, range [1, 31]"},
    {"tm_hour", "hours, range [0, 23]"},
    {"tm_min", "minutes, range [0, 59]"},
    {"tm_sec", "seconds, range [0, 61])"},
    {"tm_wday", "day of week, range [0, 6], Monday is 0"},
    {"tm_yday", "day of year, range [1, 366]"},
    {"tm_isdst", "1 if summer time is in effect, 0 if not, and -1 if unknown"},
    {0}
};

static PyStructSequence_Desc struct_time_type_desc = {
    "time.struct_time",
    "The time value as returned by gmtime(), localtime(), and strptime(), and\n"
    " accepted by asctime(), mktime() and strftime().  May be considered as a\n"
    " sequence of 9 integers.\n\n"
    " Note that several fields' values are not the same as those defined by\n"
    " the C language standard for struct tm.  For example, the value of the\n"
    " field tm_year is the actual year, not year - 1900.  See individual\n"
    " fields' descriptions for details.",
    struct_time_type_fields,
    9,
};

static int initialized;
static PyTypeObject StructTimeType;

static PyObject *
tmtotuple(struct tm *p)
{
    PyObject *v = PyStructSequence_New(&StructTimeType);
    if (v == NULL)
        return NULL;

#define SET(i,val) PyStructSequence_SET_ITEM(v, i, PyLong_FromLong((long) val))

    SET(0, p->tm_year + 1900);
    SET(1, p->tm_mon + 1);         /* Want January == 1 */
    SET(2, p->tm_mday);
    SET(3, p->tm_hour);
    SET(4, p->tm_min);
    SET(5, p->tm_sec);
    SET(6, (p->tm_wday + 6) % 7); /* Want Monday == 0 */
    SET(7, p->tm_yday + 1);        /* Want January, 1 == 1 */
    SET(8, p->tm_isdst);
#undef SET
    if (PyErr_Occurred()) {
        Py_XDECREF(v);
        return NULL;
    }

    return v;
}

/* Parse arg tuple that can contain an optional float-or-None value;
   format needs to be "|O:name".
   Returns non-zero on success (parallels PyArg_ParseTuple).
*/
static int
parse_time_t_args(PyObject *args, char *format, time_t *pwhen)
{
    PyObject *ot = NULL;
    time_t whent;

    if (!PyArg_ParseTuple(args, format, &ot))
        return 0;
    if (ot == NULL || ot == Py_None) {
        whent = time(NULL);
    }
    else {
        if (_PyTime_ObjectToTime_t(ot, &whent) == -1)
            return 0;
    }
    *pwhen = whent;
    return 1;
}

static PyObject *
time_gmtime(PyObject *self, PyObject *args)
{
    time_t when;
    struct tm buf, *local;

    if (!parse_time_t_args(args, "|O:gmtime", &when))
        return NULL;

    errno = 0;
    local = gmtime(&when);
    if (local == NULL) {
#ifdef EINVAL
        if (errno == 0)
            errno = EINVAL;
#endif
        return PyErr_SetFromErrno(PyExc_OSError);
    }
    buf = *local;
    return tmtotuple(&buf);
}

PyDoc_STRVAR(gmtime_doc,
"gmtime([seconds]) -> (tm_year, tm_mon, tm_mday, tm_hour, tm_min,\n\
                       tm_sec, tm_wday, tm_yday, tm_isdst)\n\
\n\
Convert seconds since the Epoch to a time tuple expressing UTC (a.k.a.\n\
GMT).  When 'seconds' is not passed in, convert the current time instead.");

static int
pylocaltime(time_t *timep, struct tm *result)
{
    struct tm *local;

    assert (timep != NULL);
    local = localtime(timep);
    if (local == NULL) {
        /* unconvertible time */
#ifdef EINVAL
        if (errno == 0)
            errno = EINVAL;
#endif
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    *result = *local;
    return 0;
}

static PyObject *
time_localtime(PyObject *self, PyObject *args)
{
    time_t when;
    struct tm buf;

    if (!parse_time_t_args(args, "|O:localtime", &when))
        return NULL;
    if (pylocaltime(&when, &buf) == 1)
        return NULL;
    return tmtotuple(&buf);
}

PyDoc_STRVAR(localtime_doc,
"localtime([seconds]) -> (tm_year,tm_mon,tm_mday,tm_hour,tm_min,\n\
                          tm_sec,tm_wday,tm_yday,tm_isdst)\n\
\n\
Convert seconds since the Epoch to a time tuple expressing local time.\n\
When 'seconds' is not passed in, convert the current time instead.");

/* Convert 9-item tuple to tm structure.  Return 1 on success, set
 * an exception and return 0 on error.
 */
static int
gettmarg(PyObject *args, struct tm *p)
{
    int y;

    memset((void *) p, '\0', sizeof(struct tm));

    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError,
                        "Tuple or struct_time argument required");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "iiiiiiiii",
                          &y, &p->tm_mon, &p->tm_mday,
                          &p->tm_hour, &p->tm_min, &p->tm_sec,
                          &p->tm_wday, &p->tm_yday, &p->tm_isdst))
        return 0;
    p->tm_year = y - 1900;
    p->tm_mon--;
    p->tm_wday = (p->tm_wday + 1) % 7;
    p->tm_yday--;
    return 1;
}

/* Check values of the struct tm fields before it is passed to strftime() and
 * asctime().  Return 1 if all values are valid, otherwise set an exception
 * and returns 0.
 */
static int
checktm(struct tm* buf)
{
    /* Checks added to make sure strftime() and asctime() does not crash Python by
       indexing blindly into some array for a textual representation
       by some bad index (fixes bug #897625 and #6608).

       Also support values of zero from Python code for arguments in which
       that is out of range by forcing that value to the lowest value that
       is valid (fixed bug #1520914).

       Valid ranges based on what is allowed in struct tm:

       - tm_year: [0, max(int)] (1)
       - tm_mon: [0, 11] (2)
       - tm_mday: [1, 31]
       - tm_hour: [0, 23]
       - tm_min: [0, 59]
       - tm_sec: [0, 60]
       - tm_wday: [0, 6] (1)
       - tm_yday: [0, 365] (2)
       - tm_isdst: [-max(int), max(int)]

       (1) gettmarg() handles bounds-checking.
       (2) Python's acceptable range is one greater than the range in C,
       thus need to check against automatic decrement by gettmarg().
    */
    if (buf->tm_mon == -1)
        buf->tm_mon = 0;
    else if (buf->tm_mon < 0 || buf->tm_mon > 11) {
        PyErr_SetString(PyExc_ValueError, "month out of range");
        return 0;
    }
    if (buf->tm_mday == 0)
        buf->tm_mday = 1;
    else if (buf->tm_mday < 0 || buf->tm_mday > 31) {
        PyErr_SetString(PyExc_ValueError, "day of month out of range");
        return 0;
    }
    if (buf->tm_hour < 0 || buf->tm_hour > 23) {
        PyErr_SetString(PyExc_ValueError, "hour out of range");
        return 0;
    }
    if (buf->tm_min < 0 || buf->tm_min > 59) {
        PyErr_SetString(PyExc_ValueError, "minute out of range");
        return 0;
    }
    if (buf->tm_sec < 0 || buf->tm_sec > 61) {
        PyErr_SetString(PyExc_ValueError, "seconds out of range");
        return 0;
    }
    /* tm_wday does not need checking of its upper-bound since taking
    ``% 7`` in gettmarg() automatically restricts the range. */
    if (buf->tm_wday < 0) {
        PyErr_SetString(PyExc_ValueError, "day of week out of range");
        return 0;
    }
    if (buf->tm_yday == -1)
        buf->tm_yday = 0;
    else if (buf->tm_yday < 0 || buf->tm_yday > 365) {
        PyErr_SetString(PyExc_ValueError, "day of year out of range");
        return 0;
    }
    return 1;
}

#ifdef MS_WINDOWS
   /* wcsftime() doesn't format correctly time zones, see issue #10653 */
#  undef HAVE_WCSFTIME
#endif

#ifdef HAVE_STRFTIME
#ifdef HAVE_WCSFTIME
#define time_char wchar_t
#define format_time wcsftime
#define time_strlen wcslen
#else
#define time_char char
#define format_time strftime
#define time_strlen strlen
#endif

static PyObject *
time_strftime(PyObject *self, PyObject *args)
{
    PyObject *tup = NULL;
    struct tm buf;
    const time_char *fmt;
#ifdef HAVE_WCSFTIME
    wchar_t *format;
#else
    PyObject *format;
#endif
    PyObject *format_arg;
    size_t fmtlen, buflen;
    time_char *outbuf = NULL;
    size_t i;
    PyObject *ret = NULL;

    memset((void *) &buf, '\0', sizeof(buf));

    /* Will always expect a unicode string to be passed as format.
       Given that there's no str type anymore in py3k this seems safe.
    */
    if (!PyArg_ParseTuple(args, "U|O:strftime", &format_arg, &tup))
        return NULL;

    if (tup == NULL) {
        time_t tt = time(NULL);
        if (pylocaltime(&tt, &buf) == -1)
            return NULL;
    }
    else if (!gettmarg(tup, &buf) || !checktm(&buf))
        return NULL;

#if defined(_MSC_VER) || defined(sun)
    if (buf.tm_year + 1900 < 1 || 9999 < buf.tm_year + 1900) {
        PyErr_SetString(PyExc_ValueError,
                        "strftime() requires year in [1; 9999]");
        return NULL;
    }
#endif

    /* Normalize tm_isdst just in case someone foolishly implements %Z
       based on the assumption that tm_isdst falls within the range of
       [-1, 1] */
    if (buf.tm_isdst < -1)
        buf.tm_isdst = -1;
    else if (buf.tm_isdst > 1)
        buf.tm_isdst = 1;

#ifdef HAVE_WCSFTIME
    format = PyUnicode_AsWideCharString(format_arg, NULL);
    if (format == NULL)
        return NULL;
    fmt = format;
#else
    /* Convert the unicode string to an ascii one */
    format = PyUnicode_EncodeLocale(format_arg, "surrogateescape");
    if (format == NULL)
        return NULL;
    fmt = PyBytes_AS_STRING(format);
#endif

#if defined(MS_WINDOWS) && !defined(HAVE_WCSFTIME)
    /* check that the format string contains only valid directives */
    for(outbuf = strchr(fmt, '%');
        outbuf != NULL;
        outbuf = strchr(outbuf+2, '%'))
    {
        if (outbuf[1]=='#')
            ++outbuf; /* not documented by python, */
        if (outbuf[1]=='\0' ||
            !strchr("aAbBcdHIjmMpSUwWxXyYzZ%", outbuf[1]))
        {
            PyErr_SetString(PyExc_ValueError, "Invalid format string");
            Py_DECREF(format);
            return NULL;
        }
    }
#endif

    fmtlen = time_strlen(fmt);

    /* I hate these functions that presume you know how big the output
     * will be ahead of time...
     */
    for (i = 1024; ; i += i) {
#if defined _MSC_VER && _MSC_VER >= 1400 && defined(__STDC_SECURE_LIB__)
        int err;
#endif
        outbuf = (time_char *)PyMem_Malloc(i*sizeof(time_char));
        if (outbuf == NULL) {
            PyErr_NoMemory();
            break;
        }
        buflen = format_time(outbuf, i, fmt, &buf);
#if defined _MSC_VER && _MSC_VER >= 1400 && defined(__STDC_SECURE_LIB__)
        err = errno;
#endif
        if (buflen > 0 || i >= 256 * fmtlen) {
            /* If the buffer is 256 times as long as the format,
               it's probably not failing for lack of room!
               More likely, the format yields an empty result,
               e.g. an empty format, or %Z when the timezone
               is unknown. */
#ifdef HAVE_WCSFTIME
            ret = PyUnicode_FromWideChar(outbuf, buflen);
#else
            ret = PyUnicode_DecodeLocaleAndSize(outbuf, buflen,
                                                "surrogateescape");
#endif
            PyMem_Free(outbuf);
            break;
        }
        PyMem_Free(outbuf);
#if defined _MSC_VER && _MSC_VER >= 1400 && defined(__STDC_SECURE_LIB__)
        /* VisualStudio .NET 2005 does this properly */
        if (buflen == 0 && err == EINVAL) {
            PyErr_SetString(PyExc_ValueError, "Invalid format string");
            break;
        }
#endif
    }
#ifdef HAVE_WCSFTIME
    PyMem_Free(format);
#else
    Py_DECREF(format);
#endif
    return ret;
}

#undef time_char
#undef format_time

PyDoc_STRVAR(strftime_doc,
"strftime(format[, tuple]) -> string\n\
\n\
Convert a time tuple to a string according to a format specification.\n\
See the library reference manual for formatting codes. When the time tuple\n\
is not present, current time as returned by localtime() is used.");
#endif /* HAVE_STRFTIME */

static PyObject *
time_strptime(PyObject *self, PyObject *args)
{
    PyObject *strptime_module = PyImport_ImportModuleNoBlock("_strptime");
    PyObject *strptime_result;
    _Py_IDENTIFIER(_strptime_time);

    if (!strptime_module)
        return NULL;
    strptime_result = _PyObject_CallMethodId(strptime_module,
                                             &PyId__strptime_time, "O", args);
    Py_DECREF(strptime_module);
    return strptime_result;
}


PyDoc_STRVAR(strptime_doc,
"strptime(string, format) -> struct_time\n\
\n\
Parse a string to a time tuple according to a format specification.\n\
See the library reference manual for formatting codes (same as strftime()).");

static PyObject *
_asctime(struct tm *timeptr)
{
    /* Inspired by Open Group reference implementation available at
     * http://pubs.opengroup.org/onlinepubs/009695399/functions/asctime.html */
    static char wday_name[7][4] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static char mon_name[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return PyUnicode_FromFormat(
        "%s %s%3d %.2d:%.2d:%.2d %d",
        wday_name[timeptr->tm_wday],
        mon_name[timeptr->tm_mon],
        timeptr->tm_mday, timeptr->tm_hour,
        timeptr->tm_min, timeptr->tm_sec,
        1900 + timeptr->tm_year);
}

static PyObject *
time_asctime(PyObject *self, PyObject *args)
{
    PyObject *tup = NULL;
    struct tm buf;

    if (!PyArg_UnpackTuple(args, "asctime", 0, 1, &tup))
        return NULL;
    if (tup == NULL) {
        time_t tt = time(NULL);
        if (pylocaltime(&tt, &buf) == -1)
            return NULL;

    } else if (!gettmarg(tup, &buf) || !checktm(&buf))
        return NULL;
    return _asctime(&buf);
}

PyDoc_STRVAR(asctime_doc,
"asctime([tuple]) -> string\n\
\n\
Convert a time tuple to a string, e.g. 'Sat Jun 06 16:26:11 1998'.\n\
When the time tuple is not present, current time as returned by localtime()\n\
is used.");

static PyObject *
time_ctime(PyObject *self, PyObject *args)
{
    time_t tt;
    struct tm buf;
    if (!parse_time_t_args(args, "|O:ctime", &tt))
        return NULL;
    if (pylocaltime(&tt, &buf) == -1)
        return NULL;
    return _asctime(&buf);
}

PyDoc_STRVAR(ctime_doc,
"ctime(seconds) -> string\n\
\n\
Convert a time in seconds since the Epoch to a string in local time.\n\
This is equivalent to asctime(localtime(seconds)). When the time tuple is\n\
not present, current time as returned by localtime() is used.");

#ifdef HAVE_MKTIME
static PyObject *
time_mktime(PyObject *self, PyObject *tup)
{
    struct tm buf;
    time_t tt;
    if (!gettmarg(tup, &buf))
        return NULL;
    buf.tm_wday = -1;  /* sentinel; original value ignored */
    tt = mktime(&buf);
    /* Return value of -1 does not necessarily mean an error, but tm_wday
     * cannot remain set to -1 if mktime succeeded. */
    if (tt == (time_t)(-1) && buf.tm_wday == -1) {
        PyErr_SetString(PyExc_OverflowError,
                        "mktime argument out of range");
        return NULL;
    }
    return PyFloat_FromDouble((double)tt);
}

PyDoc_STRVAR(mktime_doc,
"mktime(tuple) -> floating point number\n\
\n\
Convert a time tuple in local time to seconds since the Epoch.");
#endif /* HAVE_MKTIME */

#ifdef HAVE_WORKING_TZSET
static void PyInit_timezone(PyObject *module);

static PyObject *
time_tzset(PyObject *self, PyObject *unused)
{
    PyObject* m;

    m = PyImport_ImportModuleNoBlock("time");
    if (m == NULL) {
        return NULL;
    }

    tzset();

    /* Reset timezone, altzone, daylight and tzname */
    PyInit_timezone(m);
    Py_DECREF(m);

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(tzset_doc,
"tzset()\n\
\n\
Initialize, or reinitialize, the local timezone to the value stored in\n\
os.environ['TZ']. The TZ environment variable should be specified in\n\
standard Unix timezone format as documented in the tzset man page\n\
(eg. 'US/Eastern', 'Europe/Amsterdam'). Unknown timezones will silently\n\
fall back to UTC. If the TZ environment variable is not set, the local\n\
timezone is set to the systems best guess of wallclock time.\n\
Changing the TZ environment variable without calling tzset *may* change\n\
the local timezone used by methods such as localtime, but this behaviour\n\
should not be relied on.");
#endif /* HAVE_WORKING_TZSET */

static PyObject*
steady_clock(int strict)
{
#if defined(MS_WINDOWS) && !defined(__BORLANDC__)
    return win32_clock(!strict);
#elif defined(__APPLE__)
    static mach_timebase_info_data_t timebase;
    uint64_t time;
    double secs;

    if (timebase.denom == 0) {
        /* According to the Technical Q&A QA1398, mach_timebase_info() cannot
           fail: https://developer.apple.com/library/mac/#qa/qa1398/ */
        (void)mach_timebase_info(&timebase);
    }

    time = mach_absolute_time();
    secs = (double)time * timebase.numer / timebase.denom * 1e-9;

    return PyFloat_FromDouble(secs);
#elif defined(HAVE_CLOCK_GETTIME)
    static int steady_clk_index = 0;
    static int monotonic_clk_index = 0;
    int *clk_index;
    clockid_t steady_clk_ids[] = {
#ifdef CLOCK_MONOTONIC_RAW
        CLOCK_MONOTONIC_RAW,
#endif
        CLOCK_MONOTONIC,
        CLOCK_REALTIME
    };
    clockid_t monotonic_clk_ids[] = {
#ifdef CLOCK_MONOTONIC_RAW
        CLOCK_MONOTONIC_RAW,
#endif
        CLOCK_MONOTONIC
    };
    clockid_t *clk_ids;
    int clk_ids_len;
    int ret;
    struct timespec tp;

    if (strict) {
        clk_index = &monotonic_clk_index;
        clk_ids = monotonic_clk_ids;
        clk_ids_len = Py_ARRAY_LENGTH(monotonic_clk_ids);
    }
    else {
        clk_index = &steady_clk_index;
        clk_ids = steady_clk_ids;
        clk_ids_len = Py_ARRAY_LENGTH(steady_clk_ids);
    }

    while (0 <= *clk_index) {
        clockid_t clk_id = clk_ids[*clk_index];
        ret = clock_gettime(clk_id, &tp);
        if (ret == 0)
            return PyFloat_FromDouble(tp.tv_sec + tp.tv_nsec * 1e-9);

        (*clk_index)++;
        if (clk_ids_len <= *clk_index)
            (*clk_index) = -1;
    }
    if (strict) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return floattime();
#else
    if (strict) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "no steady clock available on your platform");
        return NULL;
    }
    return floattime();
#endif
}

static PyObject *
time_steady(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"strict", NULL};
    int strict = 0;

    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "|i:steady", kwlist,
            &strict))
        return NULL;

    return steady_clock(strict);
}

PyDoc_STRVAR(steady_doc,
"steady(strict=False) -> float\n\
\n\
Return the current time as a floating point number expressed in seconds.\n\
This clock advances at a steady rate relative to real time and it may not\n\
be adjusted. The reference point of the returned value is undefined so only\n\
the difference of consecutive calls is valid.");


static void
PyInit_timezone(PyObject *m) {
    /* This code moved from PyInit_time wholesale to allow calling it from
    time_tzset. In the future, some parts of it can be moved back
    (for platforms that don't HAVE_WORKING_TZSET, when we know what they
    are), and the extraneous calls to tzset(3) should be removed.
    I haven't done this yet, as I don't want to change this code as
    little as possible when introducing the time.tzset and time.tzsetwall
    methods. This should simply be a method of doing the following once,
    at the top of this function and removing the call to tzset() from
    time_tzset():

        #ifdef HAVE_TZSET
        tzset()
        #endif

    And I'm lazy and hate C so nyer.
     */
#if defined(HAVE_TZNAME) && !defined(__GLIBC__) && !defined(__CYGWIN__)
    PyObject *otz0, *otz1;
    tzset();
#ifdef PYOS_OS2
    PyModule_AddIntConstant(m, "timezone", _timezone);
#else /* !PYOS_OS2 */
    PyModule_AddIntConstant(m, "timezone", timezone);
#endif /* PYOS_OS2 */
#ifdef HAVE_ALTZONE
    PyModule_AddIntConstant(m, "altzone", altzone);
#else
#ifdef PYOS_OS2
    PyModule_AddIntConstant(m, "altzone", _timezone-3600);
#else /* !PYOS_OS2 */
    PyModule_AddIntConstant(m, "altzone", timezone-3600);
#endif /* PYOS_OS2 */
#endif
    PyModule_AddIntConstant(m, "daylight", daylight);
    otz0 = PyUnicode_DecodeLocale(tzname[0], "surrogateescape");
    otz1 = PyUnicode_DecodeLocale(tzname[1], "surrogateescape");
    PyModule_AddObject(m, "tzname", Py_BuildValue("(NN)", otz0, otz1));
#else /* !HAVE_TZNAME || __GLIBC__ || __CYGWIN__*/
#ifdef HAVE_STRUCT_TM_TM_ZONE
    {
#define YEAR ((time_t)((365 * 24 + 6) * 3600))
        time_t t;
        struct tm *p;
        long janzone, julyzone;
        char janname[10], julyname[10];
        t = (time((time_t *)0) / YEAR) * YEAR;
        p = localtime(&t);
        janzone = -p->tm_gmtoff;
        strncpy(janname, p->tm_zone ? p->tm_zone : "   ", 9);
        janname[9] = '\0';
        t += YEAR/2;
        p = localtime(&t);
        julyzone = -p->tm_gmtoff;
        strncpy(julyname, p->tm_zone ? p->tm_zone : "   ", 9);
        julyname[9] = '\0';

        if( janzone < julyzone ) {
            /* DST is reversed in the southern hemisphere */
            PyModule_AddIntConstant(m, "timezone", julyzone);
            PyModule_AddIntConstant(m, "altzone", janzone);
            PyModule_AddIntConstant(m, "daylight",
                                    janzone != julyzone);
            PyModule_AddObject(m, "tzname",
                               Py_BuildValue("(zz)",
                                             julyname, janname));
        } else {
            PyModule_AddIntConstant(m, "timezone", janzone);
            PyModule_AddIntConstant(m, "altzone", julyzone);
            PyModule_AddIntConstant(m, "daylight",
                                    janzone != julyzone);
            PyModule_AddObject(m, "tzname",
                               Py_BuildValue("(zz)",
                                             janname, julyname));
        }
    }
#else
#endif /* HAVE_STRUCT_TM_TM_ZONE */
#ifdef __CYGWIN__
    tzset();
    PyModule_AddIntConstant(m, "timezone", _timezone);
    PyModule_AddIntConstant(m, "altzone", _timezone-3600);
    PyModule_AddIntConstant(m, "daylight", _daylight);
    PyModule_AddObject(m, "tzname",
                       Py_BuildValue("(zz)", _tzname[0], _tzname[1]));
#endif /* __CYGWIN__ */
#endif /* !HAVE_TZNAME || __GLIBC__ || __CYGWIN__*/

#if defined(HAVE_CLOCK_GETTIME) || defined(HAVE_CLOCK_GETRES)
#ifdef CLOCK_REALTIME
    PyModule_AddIntMacro(m, CLOCK_REALTIME);
#endif
#ifdef CLOCK_MONOTONIC
    PyModule_AddIntMacro(m, CLOCK_MONOTONIC);
#endif
#ifdef CLOCK_MONOTONIC_RAW
    PyModule_AddIntMacro(m, CLOCK_MONOTONIC_RAW);
#endif
#ifdef CLOCK_HIGHRES
    PyModule_AddIntMacro(m, CLOCK_HIGHRES);
#endif
#ifdef CLOCK_PROCESS_CPUTIME_ID
    PyModule_AddIntMacro(m, CLOCK_PROCESS_CPUTIME_ID);
#endif
#ifdef CLOCK_THREAD_CPUTIME_ID
    PyModule_AddIntMacro(m, CLOCK_THREAD_CPUTIME_ID);
#endif
#endif /* HAVE_CLOCK_GETTIME */
}


static PyMethodDef time_methods[] = {
    {"time",            time_time, METH_NOARGS, time_doc},
#if (defined(MS_WINDOWS) && !defined(__BORLANDC__)) || defined(HAVE_CLOCK)
    {"clock",           time_clock, METH_NOARGS, clock_doc},
#endif
#ifdef HAVE_CLOCK_GETTIME
    {"clock_gettime",   time_clock_gettime, METH_VARARGS, clock_gettime_doc},
#endif
#ifdef HAVE_CLOCK_GETRES
    {"clock_getres",    time_clock_getres, METH_VARARGS, clock_getres_doc},
#endif
    {"sleep",           time_sleep, METH_VARARGS, sleep_doc},
    {"gmtime",          time_gmtime, METH_VARARGS, gmtime_doc},
    {"localtime",       time_localtime, METH_VARARGS, localtime_doc},
    {"asctime",         time_asctime, METH_VARARGS, asctime_doc},
    {"ctime",           time_ctime, METH_VARARGS, ctime_doc},
#ifdef HAVE_MKTIME
    {"mktime",          time_mktime, METH_O, mktime_doc},
#endif
    {"steady",          (PyCFunction)time_steady, METH_VARARGS|METH_KEYWORDS,
                        steady_doc},
#ifdef HAVE_STRFTIME
    {"strftime",        time_strftime, METH_VARARGS, strftime_doc},
#endif
    {"strptime",        time_strptime, METH_VARARGS, strptime_doc},
#ifdef HAVE_WORKING_TZSET
    {"tzset",           time_tzset, METH_NOARGS, tzset_doc},
#endif
    {NULL,              NULL}           /* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module provides various functions to manipulate time values.\n\
\n\
There are two standard representations of time.  One is the number\n\
of seconds since the Epoch, in UTC (a.k.a. GMT).  It may be an integer\n\
or a floating point number (to represent fractions of seconds).\n\
The Epoch is system-defined; on Unix, it is generally January 1st, 1970.\n\
The actual value can be retrieved by calling gmtime(0).\n\
\n\
The other representation is a tuple of 9 integers giving local time.\n\
The tuple items are:\n\
  year (including century, e.g. 1998)\n\
  month (1-12)\n\
  day (1-31)\n\
  hours (0-23)\n\
  minutes (0-59)\n\
  seconds (0-59)\n\
  weekday (0-6, Monday is 0)\n\
  Julian day (day in the year, 1-366)\n\
  DST (Daylight Savings Time) flag (-1, 0 or 1)\n\
If the DST flag is 0, the time is given in the regular time zone;\n\
if it is 1, the time is given in the DST time zone;\n\
if it is -1, mktime() should guess based on the date and time.\n\
\n\
Variables:\n\
\n\
timezone -- difference in seconds between UTC and local standard time\n\
altzone -- difference in  seconds between UTC and local DST time\n\
daylight -- whether local time should reflect DST\n\
tzname -- tuple of (standard time zone name, DST time zone name)\n\
\n\
Functions:\n\
\n\
time() -- return current time in seconds since the Epoch as a float\n\
clock() -- return CPU time since process start as a float\n\
sleep() -- delay for a number of seconds given as a float\n\
gmtime() -- convert seconds since Epoch to UTC tuple\n\
localtime() -- convert seconds since Epoch to local time tuple\n\
asctime() -- convert time tuple to string\n\
ctime() -- convert time in seconds to string\n\
mktime() -- convert local time tuple to seconds since Epoch\n\
strftime() -- convert time tuple to string according to format specification\n\
strptime() -- parse string to time tuple according to format specification\n\
tzset() -- change the local timezone");



static struct PyModuleDef timemodule = {
    PyModuleDef_HEAD_INIT,
    "time",
    module_doc,
    -1,
    time_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_time(void)
{
    PyObject *m;
    m = PyModule_Create(&timemodule);
    if (m == NULL)
        return NULL;

    /* Set, or reset, module variables like time.timezone */
    PyInit_timezone(m);

    if (!initialized) {
        PyStructSequence_InitType(&StructTimeType,
                                  &struct_time_type_desc);
    }
    Py_INCREF(&StructTimeType);
    PyModule_AddObject(m, "struct_time", (PyObject*) &StructTimeType);
    initialized = 1;
    return m;
}

static PyObject*
floattime(void)
{
    _PyTime_timeval t;
#ifdef HAVE_CLOCK_GETTIME
    struct timespec tp;
    int ret;

    /* _PyTime_gettimeofday() does not use clock_gettime()
       because it would require to link Python to the rt (real-time)
       library, at least on Linux */
    ret = clock_gettime(CLOCK_REALTIME, &tp);
    if (ret == 0)
        return PyFloat_FromDouble(tp.tv_sec + tp.tv_nsec * 1e-9);
#endif
    _PyTime_gettimeofday(&t);
    return PyFloat_FromDouble((double)t.tv_sec + t.tv_usec * 1e-6);
}


/* Implement floatsleep() for various platforms.
   When interrupted (or when another error occurs), return -1 and
   set an exception; else return 0. */

static int
floatsleep(double secs)
{
/* XXX Should test for MS_WINDOWS first! */
#if defined(HAVE_SELECT) && !defined(__EMX__)
    struct timeval t;
    double frac;
    int err;

    frac = fmod(secs, 1.0);
    secs = floor(secs);
    t.tv_sec = (long)secs;
    t.tv_usec = (long)(frac*1000000.0);
    Py_BEGIN_ALLOW_THREADS
    err = select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &t);
    Py_END_ALLOW_THREADS
    if (err != 0) {
#ifdef EINTR
        if (errno == EINTR) {
            if (PyErr_CheckSignals())
                return -1;
        }
        else
#endif
        {
            PyErr_SetFromErrno(PyExc_IOError);
            return -1;
        }
    }
#elif defined(__WATCOMC__) && !defined(__QNX__)
    /* XXX Can't interrupt this sleep */
    Py_BEGIN_ALLOW_THREADS
    delay((int)(secs * 1000 + 0.5));  /* delay() uses milliseconds */
    Py_END_ALLOW_THREADS
#elif defined(MS_WINDOWS)
    {
        double millisecs = secs * 1000.0;
        unsigned long ul_millis;

        if (millisecs > (double)ULONG_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "sleep length is too large");
            return -1;
        }
        Py_BEGIN_ALLOW_THREADS
        /* Allow sleep(0) to maintain win32 semantics, and as decreed
         * by Guido, only the main thread can be interrupted.
         */
        ul_millis = (unsigned long)millisecs;
        if (ul_millis == 0 || !_PyOS_IsMainThread())
            Sleep(ul_millis);
        else {
            DWORD rc;
            HANDLE hInterruptEvent = _PyOS_SigintEvent();
            ResetEvent(hInterruptEvent);
            rc = WaitForSingleObject(hInterruptEvent, ul_millis);
            if (rc == WAIT_OBJECT_0) {
                Py_BLOCK_THREADS
                errno = EINTR;
                PyErr_SetFromErrno(PyExc_IOError);
                return -1;
            }
        }
        Py_END_ALLOW_THREADS
    }
#elif defined(PYOS_OS2)
    /* This Sleep *IS* Interruptable by Exceptions */
    Py_BEGIN_ALLOW_THREADS
    if (DosSleep(secs * 1000) != NO_ERROR) {
        Py_BLOCK_THREADS
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }
    Py_END_ALLOW_THREADS
#else
    /* XXX Can't interrupt this sleep */
    Py_BEGIN_ALLOW_THREADS
    sleep((int)secs);
    Py_END_ALLOW_THREADS
#endif

    return 0;
}
