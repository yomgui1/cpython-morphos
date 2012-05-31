/*******************************************************************************
 *** File: common.h
 *** Author: Guillaume Roguez (aka Yomgui)
 *** Date (YYYY/MM/DD): 20050216
 ***
 ***
 *** Description:
 ***
 ***    Common header for all emulation .c files.
 ***
 ***
 *** History:
 ***
 *** Date     | Author       | Desciption of modifications
 *** ---------|--------------|--------------------------------------------------
 *** 20050216 | Yomgui       | Initial Release.
 *** 20050216 | Yomgui       | New debug flags => MORPHOS_EMUL_DEBUG.
 *** 20050223 | Yomgui       | New includes.
 *** 20050406 | Yomgui       | Add EMUL_FILE_FLAGS.
 *** 20050421 | Yomgui       | Add PyMorphos_POpenSubTaskMsg structure.
 *** 20070107 | Yomgui       | Add VALID_FD macro.
 ***
 *******************************************************************************
 */

#ifndef COMMON_H
#define COMMON_H

//#undef NDEBUG

/*
** Project Includes
*/

#include "morphos.h"
#include "mosdebug.h"   


/*
** System Includes
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <errno.h>

#include <proto/exec.h>
#include <proto/dos.h>


/*
** Public Macros and Definitions
*/

//#define PYMOS_EMUL_DEBUG

#ifdef PYMOS_EMUL_DEBUG
#define EMUL_DPRINT DPRINT
#define EMUL_DPRINT_ERROR DPRINT_ERROR
#else
#define EMUL_DPRINT(x, a...)
#define EMUL_DPRINT_ERROR(x, a...)
#endif /* PYMOS_STDIO_DEBUG */

#define EMUL_FILE_FLAGS 0x8000 // FILEs with this flags are handled by our emulation routines

// not exact... think to seach the real value one day.
#define SECONDS_BETWEEN_1970_TO_1978 ((8*365+2) * 24 * 60 * 60)

// don't be disturbed by any macros

#undef malloc
#undef free
#undef realloc
#undef memcpy

#define VALID_FD(fd) (((fd) >= 0) && ((fd) < FD_SETSIZE))

/* popen subtask msg */
typedef struct
{
    struct Message      stm_SysMsg;     // system message
    APTR                stm_SubTask;    // subtask
    FILE *              stm_Fh;         // FileHandle associated
    CONST_STRPTR        stm_PipeName;   // Filename of pipe
    int                 stm_Return;     // Returned code value
    LONG                stm_IoErr;      // IoErr() value in case of error
} PyMorphos_POpenSubTaskMsg;


/*
** Public Function Prototypes
*/

extern void __seterrno(void); // this function set errno depend on IoErr() return value


#endif /* COMMON_H */
