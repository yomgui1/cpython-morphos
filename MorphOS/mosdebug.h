/*******************************************************************************
 *** $Id:$
 ***
 *** Morphos serial debug functions
 */

#ifndef MOSDEBUG_H
#define MOSDEBUG_H 1

/*
** System Includes
*/

#include <exec/types.h>
#include <clib/debug_protos.h>


/*
** Public Macros and Defines
*/

#define DB kprintf

#define FILE_SIZE       "30"
#define FUNCTION_SIZE   "32"

#ifndef NDEBUG
    // don't use following functions in functions without stack
    #define DPRINTRAW(a...) kprintf(a)

    #define DFUNC() ({ kprintf("%-"FILE_SIZE"s[%4lu]/%-"FUNCTION_SIZE"s", __FILE__, __LINE__, __FUNCTION__); })

    #define DPRINT(a...) \
        ({ DFUNC(); kprintf(": "); kprintf(a); })

    #define DPRINT_ERROR(a...) \
        ({ DFUNC(); kprintf(": [ERROR] "); kprintf(a); })

    #define DASSERT(c, f, a...) \
        ({ if (!(c)) { DFUNC(); kprintf(": [ASSERT] "); kprintf((f) , ## a);} })

    // use following functions only in pure ASM funtions
    #define ASM_DPRINTRAW8(fmt, a, b, c, d, e, f, g, h) ({ \
        __asm ("mr 3, %0": :"r"((fmt)):"r3"); \
        __asm ("mr 4, %0": :"r"((a)):"r4"); \
        __asm ("mr 5, %0": :"r"((b)):"r5"); \
        __asm ("mr 6, %0": :"r"((c)):"r6"); \
        __asm ("mr 7, %0": :"r"((d)):"r7"); \
        __asm ("mr 8, %0": :"r"((e)):"r8"); \
        __asm ("mr 9, %0": :"r"((f)):"r9"); \
        __asm ("mr 10, %0": :"r"((g)):"r10"); \
        __asm ("mr 11, %0": :"r"((h)):"r11"); \
        __asm ("crclr 4*cr1+eq"); \
        __asm ("bl kprintf"); })

    #define ASM_DPRINTRAW(f) ({ \
        __asm ("mr 3, %0": :"r"((f)):"r3"); \
        __asm ("crclr 4*cr1+eq"); \
        __asm ("bl kprintf"); })

    #define ASM_DFUNC() ASM_DPRINTRAW8("%-"FILE_SIZE"s[%4lu]/%-"FUNCTION_SIZE"s", __FILE__, __LINE__, __FUNCTION__, 0, 0, 0, 0, 0);

    #define ASM_DPRINT(f, a...) \
        ({ ASM_DFUNC(); ASM_DPRINTRAW(": "); ASM_DPRINTRAW8((f), ## a, 0, 0, 0, 0, 0, 0, 0); })

    #define ASM_DPRINT_ERROR(f, a...) \
        ({ DFUNC(); ASM_DPRINTRAW(": [ERROR] "); ASM_DPRINTRAW8((f), ## a, 0, 0, 0, 0, 0, 0, 0); })

    #define ASM_DASSERT(c, f) \
        ({ if (!(c)) { ASM_DFUNC(); ASM_DPRINTRAW(": [ASSERT] "); ASM_DPRINTRAW8((f), ## a, 0, 0, 0, 0, 0, 0, 0, 0);} })

    // predefined debug messages
    #define DMSG_NOMEM() DPRINT_ERROR("insuffisent memory\n")
    #define DMSG_NOTIMPL() DPRINT_ERROR("not implemented\n")
#else
    #define DPRINTRAW(a...)
    #define DPRINT(a...)
    #define DPRINT_ERROR(a...)
    #define DASSERT(c, m, a...)
    #define DMSG_NOMEM()
    #define DMSG_NOTIMPL()
    #define ASM_DPRINTRAW8(a...)
    #define ASM_DPRINTRAW(a...)
    #define ASM_DFUNC(a...)
    #define ASM_DPRINT(a...)
    #define ASM_DPRINT_ERROR(a...)
    #define ASM_DASSERT(a...)
#endif // NDEBUG

#endif // MOSDEBUG_H
