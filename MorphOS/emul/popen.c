/*******************************************************************************
 *** File: popen.c
 *** Author: Guillaume Roguez (aka Yomgui)
 *** Date (YYYY/MM/DD): 20050420
 ***
 ***
 *** Description:
 ***
 ***    Emulation of popen()/pclose() POSIX functions.
 ***
 *** History:
 ***
 *** Date     | Author       | Desciption of modifications
 *** ---------|--------------|--------------------------------------------------
 *** 20050420 | Yomgui       | Initial Release.
 *** 20050421 | Yomgui       | new implementation to get the return code.
 *** 20050422 | Yomgui       | first working version.
 ***
 *******************************************************************************
 */

/* 

popen(3S)                                                         popen(3S)

 NAME
      popen(), pclose() - initiate pipe I/O to/from a process

 SYNOPSIS
      #include <stdio.h>

      FILE *popen(const char *command, const char *type);

      int pclose(FILE *stream);

 DESCRIPTION
      popen() creates a pipe between the calling program and a command to be
      executed by the POSIX shell, /usr/bin/sh (see sh-posix(1)).

      The arguments to popen() are pointers to null-terminated strings
      containing, respectively, a shell command line and an I/O mode, either
      r for reading or w for writing.

      popen() returns a stream pointer such that one can write to the
      standard input of the command if the I/O mode is w by writing to the
      file stream; and one can read from the standard output of the command
      if the I/O mode is r by reading from the file stream.

      A stream opened by popen() should be closed by pclose(), which waits
      for the associated process to terminate and returns the exit status of
      the command.

      Because open files are shared, a type r command can be used as an
      input filter and a type w command as an output filter.

 APPLICATION USAGE
      popen() and pclose() are thread-safe.  These interfaces are not
      async-cancel-safe. A cancellation point may occur when a thread is
      executing popen() or pclose().

 RETURN VALUE
      popen() returns a NULL pointer if files or processes cannot be
      created.  The success of the command execution can be checked by
      examining the return value of pclose().

      pclose() returns -1 if stream is not associated with a popen()ed
      command, or 127 if /usr/bin/sh could not be executed for some reason.

 WARNINGS
      If the original and popen()ed processes concurrently read or write a
      common file, neither should use buffered I/O because the buffering
      will not work properly.  Problems with an output filter can be
      forestalled by careful buffer flushing, e.g., with fflush(); see
      fclose(3S).
*/

/*
** Project Includes
*/

#include "common.h"
#include "morphos.h"


/*
** System Includes
*/

#include <dos/dostags.h>

#include <proto/utility.h>


/*
** Private Macros and Definitions
*/

#undef popen
#undef fileno
#undef pclose

#define PIPE_NAME_MAX_LEN (14 + 8 + 1)
#define SUBTASK_NAME "Python [popen() subtask]"
#define SUBTASK_PRIORITY 0
#define SUBTASK_STACKSIZE 8192


/*
** Private Types and Structures
*/

// Following structure is read-only for the child and read-write for the parent task.
typedef struct
{
    struct MinNode              st_Node;        // node in sv_tasksList
    struct Process *            st_Proc;        // process himself
    struct MsgPort *            st_MsgPort;     // allocated during creation for the child
    PyMorphos_POpenSubTaskMsg   st_StartMsg;    // startup message, replyed at sub-task exit
} _SubTask;


/*
** Public Prototypes
*/

extern void popenSubtask(void);
extern void __seterrno(void); // this function set errno depend on IoErr() return value


/*
** Private Functions
*/
 
//+ createSubTask
static _SubTask * createSubTask(const char * cmd, char mode, const char *pipe, FILE *fh)
{
    _SubTask * st = NULL;
    struct MsgPort * port = GET_THREAD_DATA(GET_THREAD_DATA_PTR(), POpenPort);

    // create sub-task
    st = malloc(sizeof(_SubTask));
    if (st)
    {
        // set system message structure of startup message
        st->st_StartMsg.stm_SysMsg.mn_Node.ln_Type = NT_MESSAGE;
        st->st_StartMsg.stm_SysMsg.mn_ReplyPort = port;
        st->st_StartMsg.stm_SysMsg.mn_Length = sizeof(PyMorphos_POpenSubTaskMsg);
        st->st_StartMsg.stm_SubTask = st;
        st->st_StartMsg.stm_Fh = fh;
        st->st_StartMsg.stm_PipeName = pipe;

        DPRINT("creating new popen subtask <0x%08X>...\n", st);

        st->st_Proc = CreateNewProcTags(
            NP_CodeType,        CODETYPE_PPC,
            NP_Entry,           (ULONG) popenSubtask,
            NP_StartupMsg,      (ULONG) &st->st_StartMsg,
            NP_TaskMsgPort,     (ULONG) &st->st_MsgPort,
            NP_PPCStackSize,    SUBTASK_STACKSIZE,
            NP_Priority,        SUBTASK_PRIORITY,
            NP_Name,            (ULONG) SUBTASK_NAME,
            NP_Cli,             TRUE,
            NP_PPC_Arg1,        (ULONG) cmd,
            NP_PPC_Arg2,        mode,
            TAG_DONE);

        if (!st->st_Proc)
        {
            DPRINT_ERROR("can't create a new process\n");
            free(st);
            return NULL;
        }
    }
    else
        DPRINT_ERROR("can't allocate a subtask\n");

    return st;
}
//-


/*
** Public Functions
*/

//+ popen
FILE* popen(const char *cmd, const char *mode)
{
    if (cmd)
    {
        char *name;
        FILE *file;
 
        name = malloc(PIPE_NAME_MAX_LEN + 1);
        if (name)
        {
            sprintf(name, "PIPE:py_popen_%lx", GetUniqueID());
            
            file = fopen(name, mode);
            if (file)
            {
                APTR st = NULL;

                st = createSubTask(cmd, mode[0], name, file);
                if (st)
                    return file;

                DPRINT_ERROR("can't create the subtask\n");
                
                errno = EAGAIN;
                fclose(file);
            }
            else
            {
                DPRINT_ERROR("can't open '%s'\n", name);
                errno = ENOENT;
            }

            free(name);
        }
        else
        {
            DMSG_NOMEM();
            errno = ENOMEM;
        }
    }
    else
        errno = ENOENT;

    return NULL;
}
//-
//+ pclose
int pclose(FILE *stream)
{
    PyMorphos_POpenSubTaskMsg *msg;
    int result = -1;

    if (NULL != stream) {
        struct MsgPort *port = GET_THREAD_DATA(GET_THREAD_DATA_PTR(), POpenPort);

        for (;;) {
            // waiting about end of popen subtask
            WaitPort(port);
            
            msg = (APTR) GetMsg(port);
            if (msg->stm_Fh == stream) {
                result = msg->stm_Return;
                DPRINT("returned value: %ld\n", result);

                // Check for dos error
                if (result != RETURN_OK) {
                    SetIoErr(msg->stm_IoErr);
                    __seterrno();
                } else
                    result = 0;

                free((APTR)msg->stm_PipeName);
                free(msg->stm_SubTask);
            } else
                DPRINT_ERROR("Fh differs\n");

            // ok, get out !
            break;
        }

        fclose(stream);   
    } else
        errno = EINVAL;

    return result;
}
//-
