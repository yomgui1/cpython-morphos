/*******************************************************************************
 *** File: utime.c
 *** Author: Guillaume Roguez (aka Yomgui)
 *** Date (YYYY/MM/DD): 200500301
 ***
 ***
 *** Description:
 ***
 ***    Emulation of utime() unix function.
 ***
 ***
 *** History:
 ***
 *** Date     | Author       | Desciption of modifications
 *** ---------|--------------|--------------------------------------------------
 *** 20050301 | Yomgui       | Initial Release.
 ***
 *******************************************************************************
 */

/*
** Project Includes
*/

#include "common.h"


/*
** Private Macros and Definitions
*/

// don't be disturbed by any macros

#undef utime


/*
** Private Functions
*/

/*! utime()
*/
int utime( const char *name, const struct utimbuf *times )
{
    struct DateStamp stamp;
    unsigned long days, secs;
    time_t time;

    if (times == NULL)
        DateStamp(&stamp);
    else
    {
      /*
       * AmigaDOS file date is the modification time
       */
      time = times->modtime;

      /*
       * Convert time (secs since 1.1.1970 GMT) to
       * AmigaDOS DateStamp (based on 1.1.1978 local time).
       */
      time -= SECONDS_BETWEEN_1970_TO_1978; /* GMT to local */
      days = (unsigned long)time / (unsigned long)(24*60*60);
      secs = (unsigned long)time % (unsigned long)(24*60*60);
      stamp.ds_Days = (LONG)days;
      stamp.ds_Minute = (LONG)(secs / 60);
      stamp.ds_Tick = (LONG)((secs % 60) * TICKS_PER_SECOND);
    }

    if (!SetFileDate((STRPTR)name, &stamp))
    {
        __seterrno();
        return -1;
    }

    return 0;
}///

