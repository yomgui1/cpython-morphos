#include "Python.h"
#ifdef MS_WINDOWS
#include <windows.h>
#endif

#if defined(__APPLE__) && defined(HAVE_GETTIMEOFDAY) && defined(HAVE_FTIME)
  /*
   * _PyTime_gettimeofday falls back to ftime when getttimeofday fails because the latter
   * might fail on some platforms. This fallback is unwanted on MacOSX because
   * that makes it impossible to use a binary build on OSX 10.4 on earlier
   * releases of the OS. Therefore claim we don't support ftime.
   */
# undef HAVE_FTIME
#endif

#if defined(HAVE_FTIME) && !defined(MS_WINDOWS)
#include <sys/timeb.h>
extern int ftime(struct timeb *);
#endif

void
_PyTime_gettimeofday(_PyTime_timeval *tp)
{
#ifdef MS_WINDOWS
    FILETIME system_time;
    ULARGE_INTEGER large;
    ULONGLONG microseconds;

    GetSystemTimeAsFileTime(&system_time);
    large.u.LowPart = system_time.dwLowDateTime;
    large.u.HighPart = system_time.dwHighDateTime;
    /* 11,644,473,600,000,000: number of microseconds between
       the 1st january 1601 and the 1st january 1970 (369 years + 89 leap
       days). */
    microseconds = large.QuadPart / 10 - 11644473600000000;
    tp->tv_sec = microseconds / 1000000;
    tp->tv_usec = microseconds % 1000000;
#else
    /* There are three ways to get the time:
      (1) gettimeofday() -- resolution in microseconds
      (2) ftime() -- resolution in milliseconds
      (3) time() -- resolution in seconds
      In all cases the return value in a timeval struct.
      Since on some systems (e.g. SCO ODT 3.0) gettimeofday() may
      fail, so we fall back on ftime() or time().
      Note: clock resolution does not imply clock accuracy! */

#ifdef HAVE_GETTIMEOFDAY
#ifdef GETTIMEOFDAY_NO_TZ
    if (gettimeofday(tp) == 0)
        return;
#else /* !GETTIMEOFDAY_NO_TZ */
    if (gettimeofday(tp, (struct timezone *)NULL) == 0)
        return;
#endif /* !GETTIMEOFDAY_NO_TZ */
#endif /* !HAVE_GETTIMEOFDAY */

#if defined(HAVE_FTIME)
    {
        struct timeb t;
        ftime(&t);
        tp->tv_sec = t.time;
        tp->tv_usec = t.millitm * 1000;
    }
#else /* !HAVE_FTIME */
    tp->tv_sec = time(NULL);
    tp->tv_usec = 0;
#endif /* !HAVE_FTIME */

#endif /* MS_WINDOWS */
}

int
_PyTime_ObjectToTimespec(PyObject *obj, time_t *sec, long *nsec)
{
    if (PyFloat_Check(obj)) {
        double d, intpart, floatpart, err;

        d = PyFloat_AsDouble(obj);
        floatpart = modf(d, &intpart);
        if (floatpart < 0) {
            floatpart = 1.0 + floatpart;
            intpart -= 1.0;
        }

        *sec = (time_t)intpart;
        err = intpart - (double)*sec;
        if (err <= -1.0 || err >= 1.0)
            goto overflow;

        floatpart *= 1e9;
        *nsec = (long)floatpart;
        return 0;
    }
    else {
#if defined(HAVE_LONG_LONG) && SIZEOF_TIME_T == SIZEOF_LONG_LONG
        *sec = PyLong_AsLongLong(obj);
#else
        assert(sizeof(time_t) <= sizeof(long));
        *sec = PyLong_AsLong(obj);
#endif
        if (*sec == -1 && PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError))
                goto overflow;
            else
                return -1;
        }
        *nsec = 0;
        return 0;
    }

overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "timestamp out of range for platform time_t");
    return -1;
}

void
_PyTime_Init()
{
    /* Do nothing.  Needed to force linking. */
}
