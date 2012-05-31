/*******************************************************************************
 *** $Id$
 ***
 *** Morphos Library Header (.c)
 ***
 *******************************************************************************
 */

/*
** Project Includes
*/

#define PYTHON_LIBTABLE
#include "libheader.h"
#include "libprotos.h"

static const ULONG InitTable[];
static const UBYTE VersionString[];
static const UBYTE LibName[];

/*
** Public Variables
*/

struct Resident LibResident =
{
    RTC_MATCHWORD,
    (struct Resident *)&LibResident,
    (struct Resident *)&LibResident+1,
    RTF_AUTOINIT | RTF_PPC | RTF_EXTENDED,
    LIBRARY_VERSION,
    NT_LIBRARY,
    0,
    (char *)&LibName[0],
    (char *)&VersionString[7],
    (APTR)&InitTable,
    LIBRARY_REVISION,
    NULL
};


/*
** Private Variables
*/

//+ __restore_r13
/* __restore_r13()
** This function must preserve all registers except r13
*/
asm("\n"
"    .section \".text\"                                     \n"
"    .align 2                                               \n"
"    .type __restore_r13, @function                         \n"
"__restore_r13:                                             \n"
"    lwz 13, 36(12)                                         \n"
"    blr                                                    \n"
"__end__restore_r13:                                        \n"
"    .size __restore_r13, __end__restore_r13 - __restore_r13\n");
//-

static const APTR FuncTable[] =
{
    (APTR)  FUNCARRAY_BEGIN,
    (APTR)  FUNCARRAY_32BIT_NATIVE,

    (APTR)  LibOpen,
    (APTR)  LibClose,
    (APTR)  LibExpunge,
    (APTR)  LibReserved,
    (APTR)  -1,

    (APTR)  FUNCARRAY_32BIT_SYSTEMV,

    PYTHON_FUNCARRAY,

    (APTR)  FUNCARRAY_END
};

static const ULONG InitTable[] =
{
    sizeof(PythonLibrary_t),
    (ULONG) FuncTable,
    0,
    (ULONG) LibInit
};

static const UBYTE VersionString[] = "\0$VER: " LIBRARY_SHORTNAME " " LIBRARY_VERSION_STR
                                     " (" COMPILE_DATE ") MorphOS version " MORPHOS_VERSION_STR
                                     " - Ported by Guillaume Roguez";
static const UBYTE LibName[] = LIBRARY_SHORTNAME;

extern void dprintf(const char *fmt, ...);
void _t_private(void){dprintf("PYTHON BAD CALL! [%s]\n");}
