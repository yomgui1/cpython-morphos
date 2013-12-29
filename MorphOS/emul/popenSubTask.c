/*******************************************************************************
 *** File: popenSubTask.c
 *** Author: Guillaume Roguez (aka Yomgui)
 *** Date (YYYY/MM/DD): 20050421
 ***
 ***
 *** Description:
 ***
 ***    popen subtask entry.
 ***
 *** Notes: ********** WARNING **********
 ***    DON'T USE GLOBALS HERE, R13 will not be set !
 ***
 *** History:
 ***
 *** Date     | Author       | Desciption of modifications
 *** ---------|--------------|--------------------------------------------------
 *** 20050421 | Yomgui       | Initial Release.
 *** 20050422 | Yomgui       | first working version.
 ***
 *******************************************************************************
 */

/*
** Project Includes
*/

#include "common.h"

/*
** System Includes
*/

#include <dos/dostags.h>


/*
** Public Functions
*/

/*! popenSubtask()
*/
void popenSubtask(const char *cmd, const char mode)
{
    struct ExecBase * SysBase = *(APTR *)4L;
    struct DosLibrary * DOSBase;
    PyMorphos_POpenSubTaskMsg *startupMsg;
    int result = RETURN_OK;

    DPRINT("<popen subtask %p>: popen subtask launched\n", FindTask(NULL));

    DOSBase = (APTR)OpenLibrary("dos.library", 36);
    if (DOSBase != NULL)
    {
        LONG res;

        res = NewGetTaskAttrs(NULL, &startupMsg, sizeof(PyMorphos_POpenSubTaskMsg *), TASKINFOTYPE_STARTUPMSG, TAG_DONE);
        if (res && startupMsg)
        {
            const char *pipe = startupMsg->stm_PipeName;
            ULONG fh1_mode, fh2_mode;
            BPTR fh1, fh2;
 
            DPRINT("<popen subtask %p>: cmd='%s', mode='%c', pipe='%s'\n", FindTask(NULL), cmd, mode, pipe);

            fh1_mode = MODE_OLDFILE; // command will read from our pipe
            fh2_mode = MODE_NEWFILE; // command will write in "console:"

            if (mode == 'w')
            {
                ULONG tmp = fh2_mode;

                fh2_mode = fh1_mode;
                fh1_mode = tmp;
            }

            fh1 = Open("CONSOLE:", fh1_mode); // the other not used stream, redirected into "console:" 
            fh2 = Open(pipe, fh2_mode); // fh used as Input or Output for the command

            if (mode == 'w')
            {
                BPTR tmp = fh2;

                fh2 = fh1;
                fh1 = tmp;
            }

            DPRINT("<popen subtask %p>: pipe <0x%p>, CONSOLE <0x%p>\n", FindTask(NULL), fh1, fh2);

            // execute command synchronously
            if (fh1 && fh2)
            {
                result = SystemTags(cmd,
                    SYS_Input, fh1,
                    SYS_Output, fh2,
                    TAG_DONE);
            }

            if (fh1) Close(fh1);
            if (fh2) Close(fh2);

            if (result == -1)
            {
                startupMsg->stm_IoErr = IoErr();
                DPRINT("<popen subtask %p>: IoErr()=%ld\n", FindTask(NULL), IoErr());
            }
        }
        else
            DPRINT_ERROR("<popen subtask %p>: can't get startup msg\n", FindTask(NULL));
 
        CloseLibrary((APTR)DOSBase);
    }
    else
        DPRINT_ERROR("<popen subtask %p>: can't open dos.library >= 36\n", FindTask(NULL));

    DPRINT("<popen subtask %p>: result=%ld\n", FindTask(NULL), result);
    startupMsg->stm_Return = result;      
}///
