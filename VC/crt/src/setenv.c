/***
*setenv.c -set an environment variable in the environment
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines __crtsetenv() - adds a new variable to environment.
*       Internal use only.
*
*******************************************************************************/


#include <windows.h>
#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <tchar.h>
#include <rterr.h>
#include <dbgint.h>
#include <awint.h>
#include <limits.h>

#ifdef WPRFLAG
#define _tsetenvp    _wsetenvp
#else  /* WPRFLAG */
#define _tsetenvp    _setenvp
#endif  /* WPRFLAG */

/***
*int __crtsetenv(option) - add/replace/remove variable in environment
*
*Purpose:
*       option should be of the form "option=value".  If a string with the
*       given option part already exists, it is replaced with the given
*       string; otherwise the given string is added to the environment.
*       If the string is of the form "option=", then the string is
*       removed from the environment, if it exists.  If the string has
*       no equals sign, error is returned.
*
*Entry:
*       TCHAR **poption - pointer to option string to set in the environment list.
*           should be of the form "option=value".
*           This function takes ownership of this pointer in the success case.
*       int primary - Only the primary call to _crt[w]setenv needs to
*           create new copies or set the OS environment.
*           1 indicates that this is the primary call.
*
*Exit:
*       returns 0 if OK, -1 if fails.
*       If *poption is non-null on exit, we did not free it, and the caller should
*       If *poption is null on exit, we did free it, and the caller should not.
*
*Exceptions:
*
*Warnings:
*       This code will not work if variables are removed from the environment
*       by deleting them from environ[].  Use _putenv("option=") to remove a
*       variable.
*
*       The option argument will be taken ownership of by this code and may be freed!
*
*******************************************************************************/

#ifdef WPRFLAG
int __cdecl __crtwsetenv (
#else  /* WPRFLAG */
int __cdecl __crtsetenv (
#endif  /* WPRFLAG */
        _TSCHAR **poption,
        const int primary
        )
{
        int retval = 0;
        int remove; /* 1 if variable is to be removed */
        _TSCHAR *option=NULL;           /* The option string passed in */
        _TSCHAR *name, *value;
        const _TSCHAR *equal;

        /* Validate poption and dereference it */
        _VALIDATE_RETURN(poption != NULL, EINVAL, -1);

        option=*poption;

        /*
         * check that the option string is valid, find the equal sign
         * and verify '=' is not the first character in string.
         */
        if ( (option == NULL) || ((equal = _tcschr(option, _T('='))) == NULL)
            || option == equal)
        {
            errno = EINVAL;
            return -1;
        }

        /* internal consistency check: the environment string should never use buffers bigger than _MAX_ENV
         * see also SDK function SetEnvironmentVariable
         */
        _ASSERTE(equal - option < _MAX_ENV);
        _ASSERTE(_tcsnlen(equal + 1, _MAX_ENV) < _MAX_ENV);

        /* if the character following '=' is null, we are removing the
         * the environment variable. Otherwise, we are adding or updating
         * an environment variable.
         */
        remove = (*(equal + 1) == _T('\0'));


        /*
         * Update the OS environment. Don't give an error if this fails
         * since the failure will not affect the user unless he/she is making
         * direct API calls. Only need to do this for one type, OS converts
         * to other type automatically.
         */
        if ( (name = (_TSCHAR *)_calloc_crt((_tcslen(option) + 2), sizeof(_TSCHAR))) != NULL )
        {
            _ERRCHECK(_tcscpy_s(name, _tcslen(option) + 2, option));
            value = name + (equal - option);
            *value++ = _T('\0');
#ifdef _UNICODE
            if ( SetEnvironmentVariableW(name, remove ? NULL : value) == 0)
                retval = -1;
#else  /* _UNICODE */
            if (SetEnvironmentVariable(name, remove ? NULL : value) == 0)
                retval = -1;
#endif  /* _UNICODE */
            if (retval == -1)
            {
                errno = EILSEQ;
            }

            _free_crt(name);
        }
        else /* _calloc failed */
        {
            retval = -1;
        }

        /* Sync the environment variables to the same ones in Windows */
        if ( _tsetenvp() < 0 )
        {
            errno = EINVAL;
            return -1;
        }

        /*
         * At this point, the two types of environments are in sync (as much
         * as they can be anyway). The only way they can get out of sync
         * (besides users directly modifiying the environment) is if there
         * are conversion problems: If the user sets two Unicode EVs,
         * "foo1" and "foo2" and converting then to multibyte yields "foo?"
         * and "foo?", then the environment blocks will differ.
         */

        /* init env pointers */
                if(!_tenviron)
                {
                        _ASSERTE(("CRT Logic error during setenv",0));
                        return -1;
                }

        return retval;
}


