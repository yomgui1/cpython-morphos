/*******************************************************************************
 *** File: libuser.h
 *** Author: Guillaume Roguez (aka Yomgui)
 *** Date (YYYY/MM/DD): 20050203
 ***
 ***
 *** Description:
 ***
 ***   Header file of libuser.c
 ***
 ***
 *** History:
 ***
 *** Date     | Author       | Desciption of modifications
 *** ---------|--------------|--------------------------------------------------
 *** 20050205 | Yomgui       | Initial Release
 *** 20050223 | Yomgui       | Filename change (old = python_libinit.h).
 ***          |              | Remove PythonBase definition.
 *** 20050424 | Yomgui       | Return of XXX_CallCFunctionXXX().
 *** 20060917 | Yomgui       | Change naming convention for public functions.
 *** 20061004 | Yomgui       | Remove deprecaded stuff.
 ***
 *******************************************************************************
 */

#ifndef LIBUSER_H
#define LIBUSER_H


/*
** Project Includes
*/

#include "Python.h"
#include "libheader.h"


/*
** Public Function Prototypes
*/

extern SAVEDS ULONG UserLibOpen( pPythonLibrary_t );
extern SAVEDS void  UserLibClose( pPythonLibrary_t );

#endif /* LIBUSER_H */
