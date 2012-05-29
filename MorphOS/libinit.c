/*******************************************************************************
 *** $Id$
 ***
 *** Morphos Library Initializer (.c).
 ***
 *******************************************************************************
 */

/*
** Project Includes
*/

#include "mosdebug.h"

#include "libheader.h"
#include "libuser.h"


/*
** System Includes
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/lists.h>

#include <stddef.h>


/*
** Private Macros and Definitions
*/

#define R13_OFFSET 0x8000

#define MyOpenLibrary(name, s, v) (pLibBase->My##name = (APTR)OpenLibrary((s), (v)))
#define MyCloseLibrary(name) ({ if (pLibBase->My##name != NULL) {CloseLibrary((APTR)pLibBase->My##name); pLibBase->My##name = NULL;} })


/*
** Private Function Prototypes
*/

static BPTR ExpungeLib( pPythonLibrary_t );
static void UnAllocate( pPythonLibrary_t );
static ULONG __dbsize(void);


/*
** Public Variables
*/

const ULONG __abox__ = 1;
extern ULONG __sdata_size, __sbss_size;


/*
** Public Function
*/

extern int __datadata_relocs(void);

ULONG NoExecute( void ) { return -1; }
ULONG LibReserved( void ) { return 0; }

//+ __restore_r13
asm("\n"
"	 .section  \".text\"                                        \n"
"	 .align 2                                                   \n"
"	 .type  __restore_r13, @function                            \n"
"	 .globl __restore_r13                                       \n"
"__restore_r13:                                                 \n"
"	 lwz 13, 56(2)	 # MyEmulHandle->An[6] (REG_A6)             \n"
"	 lwz 13, 36(13)	 # r13 = MyLibBase->DataSeg                 \n"
"	 blr                                                        \n"
"__end__restore_r13:                                            \n"
"	 .size	 __restore_r13, __end__restore_r13 - __restore_r13  \n");
//-
//+ LibInit
struct Library * LibInit(
    APTR                pLibBase,
    BPTR                pSegList,
    struct ExecBase     *SysBase)
{
    register char *r13 asm("r13");
    pPythonLibrary_t lib = pLibBase;

    DPRINT("LibBase %p SegList 0x%lx SysBase %p\n", pLibBase, pSegList, SysBase);

    /* Load r13 register */
    asm volatile ("lis %0,__r13_init@ha; addi %0,%0,__r13_init@l" : "=r" (r13));

    lib->Library.lib_Node.ln_Pri = 0;

    lib->SegList   = pSegList;
    lib->DataSeg   = r13 - R13_OFFSET;
    lib->DataSize  = __dbsize();
    lib->Parent    = NULL;
    lib->MySysBase = SysBase;

    NEWLIST(&lib->TaskContext.TaskList);

    InitSemaphore(&lib->Semaphore);

    return pLibBase;
}
//-

#define SysBase (pLibBase->MySysBase)

//+ LibOpen
struct Library * LibOpen( void )
{
    pPythonLibrary_t    pLibBase = (pPythonLibrary_t)REG_A6;
    struct Library      *pLib = NULL;
    struct Task         *pMyTask = FindTask(NULL);
    pPythonLibrary_t    pNewBase, pChildBase;
    pPythonTaskNode_t   pChildNode;

    DPRINT("Task %p LibBase %p <%s> OpenCount %ld\n",
        pMyTask,
        pLibBase,
        pLibBase->Library.lib_Node.ln_Name,
        pLibBase->Library.lib_OpenCnt);

    /* Has this task already opened a child? */
    ForeachNode(&pLibBase->TaskContext.TaskList, pChildNode) {
        if (pChildNode->Task == pMyTask) {
            /* Yep, return it */
            pChildBase = (APTR)(((ULONG)pChildNode) - offsetof(PythonLibrary_t, TaskContext.TaskNode.Node));
            pChildBase->Library.lib_Flags &= ~LIBF_DELEXP;
            pChildBase->Library.lib_OpenCnt++;

            DPRINT("returns %p\n", &pChildBase->Library);
            return &pChildBase->Library;
        }
    }

    ObtainSemaphore(&pLibBase->Semaphore);

    if (!pLibBase->Alloc) {
        MyOpenLibrary(DOSBase, "dos.library", 39);
        MyOpenLibrary(UtilityBase, "utility.library", 39);

        if (pLibBase->MyDOSBase && pLibBase->MyUtilityBase)
            pLibBase->Alloc = TRUE;
        else
            DPRINT_ERROR("can't open needed libraries\n");
    }

    if (pLibBase->Alloc) {
        ULONG myBaseSize;

        pChildBase = NULL;
        myBaseSize = pLibBase->Library.lib_NegSize + pLibBase->Library.lib_PosSize - BASE_IGNORE_SIZE;

        pNewBase = AllocVec(myBaseSize + pLibBase->DataSize + 15, MEMF_PUBLIC | MEMF_CLEAR);
        if (pNewBase != NULL) {
            CopyMem((APTR)((ULONG)pLibBase - (ULONG)pLibBase->Library.lib_NegSize), pNewBase, myBaseSize);

            pChildBase = (APTR)((ULONG)pNewBase + (ULONG)pLibBase->Library.lib_NegSize);

            if (pLibBase->DataSize) {
                char *pOrig   = pLibBase->DataSeg;
                LONG *pRelocs = (LONG *) __datadata_relocs; /* relocs for .rodata section */
                int mem = ((int)pNewBase + myBaseSize + 15) & (unsigned int) ~15;

                CopyMem(pOrig, (char *)mem, pLibBase->DataSize);

                if (pRelocs[0] > 0) {
                    int i, numRelocs = pRelocs[0];

                    /* Map data offset on the original data space.
                     * Relocs are R_PPC_ADDR32 type (.rodata).
                     */
                    for (i = 0, pRelocs++; i < numRelocs; ++i, ++pRelocs)
                        *(long *)(mem + *pRelocs) += (int)mem - (int)pOrig;
                }

                pChildBase->DataSeg = (char *)mem + R13_OFFSET;
                DPRINT("R13=%p\n", pChildBase->DataSeg);

                if (UserLibOpen(pChildBase) == 0) {
                    DPRINT_ERROR("UserLibOpen() failed\n");

                    FreeVec(pNewBase);
                    pChildBase = NULL;
                }
            }

            if (pChildBase != NULL) {
                /* Flush JIT "cache" for the area */
		        CacheClearE(pNewBase, myBaseSize + pLibBase->DataSize + 15, CACRF_ClearI);

                pChildBase->Parent = pLibBase;
                pChildBase->Library.lib_OpenCnt = 1;

                pLibBase->Library.lib_Flags &= ~LIBF_DELEXP;
                pLibBase->Library.lib_OpenCnt++;

                /* Register which task opened this child */
                pChildBase->TaskContext.TaskNode.Task = pMyTask;
                ADDTAIL(&pLibBase->TaskContext.TaskList, &pChildBase->TaskContext.TaskNode.Node);

                pLib = (struct Library *)pChildBase;
            }
        } else
            DPRINT_ERROR("can't allocate the new base\n");
    }

    if ((pLib == NULL) && (pLibBase->Library.lib_OpenCnt == 0))
        UnAllocate(pLibBase);

    ReleaseSemaphore(&pLibBase->Semaphore);

    DPRINT("returns %p\n", pLib);
    return pLib;
}
//-
//+ LibClose
BPTR LibClose( void )
{
    pPythonLibrary_t pLibBase = (pPythonLibrary_t)REG_A6;
    BPTR pSegList = NULL;

    DPRINT("Task %p LibBase %p <%s> OpenCount %ld\n",
        FindTask(NULL),
        pLibBase,
        pLibBase->Library.lib_Node.ln_Name,
        pLibBase->Library.lib_OpenCnt);

    if (pLibBase->Parent)
    {
        pPythonLibrary_t pChildBase = pLibBase;

        if ((--pChildBase->Library.lib_OpenCnt) == 0)
        {
            pLibBase = pChildBase->Parent;

            REMOVE(&pChildBase->TaskContext.TaskNode.Node);

            UserLibClose(pChildBase);

            FreeVec((APTR)((ULONG)(pChildBase) - (ULONG)(pChildBase->Library.lib_NegSize)));
        }
        else
            return NULL;
    }

    ObtainSemaphore(&pLibBase->Semaphore);
    if (pLibBase->Library.lib_OpenCnt > 0)
        pLibBase->Library.lib_OpenCnt--;
    ReleaseSemaphore(&pLibBase->Semaphore);

    if (pLibBase->Library.lib_Flags & LIBF_DELEXP)
        pSegList = ExpungeLib(pLibBase);

    return pSegList;
}
//-
//+ LibExpunge
BPTR LibExpunge( void )
{
    pPythonLibrary_t pLibBase = (pPythonLibrary_t)REG_A6;

    DPRINT("LibBase %p <%s> OpenCount %ld\n",
        pLibBase,
        pLibBase->Library.lib_Node.ln_Name,
        pLibBase->Library.lib_OpenCnt);

    return ExpungeLib(pLibBase);
}
//-

/*
** Private Function
*/

//+ __dbsize
static __inline ULONG __dbsize(void)
{
	static const ULONG size[] = { (ULONG)&__sdata_size, (ULONG)&__sbss_size };
	return size[0] + size[1];
}
//-
//+ ExpungeLib
static BPTR ExpungeLib( pPythonLibrary_t pLibBase )
{
    BPTR pSegList = NULL;

    DPRINT("LibBase %p <%s> OpenCount %ld\n",
        pLibBase,
        pLibBase->Library.lib_Node.ln_Name,
        pLibBase->Library.lib_OpenCnt);

    ObtainSemaphore(&pLibBase->Semaphore);
    if (pLibBase->Library.lib_OpenCnt > 0)
    {
        DPRINT("set LIBF_DELEXP\n");
        pLibBase->Library.lib_Flags |= LIBF_DELEXP;
        ReleaseSemaphore(&pLibBase->Semaphore);

        return pSegList;
    }
    ReleaseSemaphore(&pLibBase->Semaphore);

    // be sure that it's the root library
    if (NULL == pLibBase->Parent)
    {
        if (pLibBase->Alloc)
            UnAllocate(pLibBase);
    }

    pSegList = pLibBase->SegList;

    /* We don't need a forbid() because Expunge and Close
     * are called with a pending forbid.
     * But let's do it for safety if somebody does it by hand.
     */
    Forbid();
    DPRINT("remove the library\n");
    Remove(&pLibBase->Library.lib_Node);
    Permit();

    DPRINT("free the library\n");
    FreeMem((APTR)((ULONG)(pLibBase) - (ULONG)(pLibBase->Library.lib_NegSize)),
        pLibBase->Library.lib_NegSize + pLibBase->Library.lib_PosSize);

    return pSegList;
}
//-
//+ UnAllocate
static void UnAllocate( pPythonLibrary_t pLibBase )
{
    DPRINT("freeing library ressources...\n");

    MyCloseLibrary(DOSBase);
    MyCloseLibrary(UtilityBase);

    pLibBase->Alloc = FALSE;
}
//-
