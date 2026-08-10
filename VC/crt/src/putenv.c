/***
*putenv.c - put an environment variable into the environment
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _putenv() - adds a new variable to environment; does not
*       change global environment, only the process' environment.
*
*******************************************************************************/


#include <windows.h>
#include <cruntime.h>
#include <internal.h>
#include <mtdll.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <dbgint.h>

int __cdecl _wputenv_helper(const wchar_t *, const wchar_t *);
int __cdecl _putenv_helper(const char *, const char *);

#ifdef WPRFLAG
#define _tputenv_helper _wputenv_helper
#else  /* WPRFLAG */
#define _tputenv_helper _putenv_helper
#endif  /* WPRFLAG */

/***
*int _putenv(option) - add/replace/remove variable in environment
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
*       char *option - option string to set in the environment list.
*           should be of the form "option=value".
*
*Exit:
*       returns 0 if OK, -1 if fails.
*
*Exceptions:
*
*Warning:
*       This code will not work if variables are removed from the
*       environment by deleting them from environ[].  Use _putenv("option=")
*       to remove a variable.
*
*******************************************************************************/

int __cdecl _tputenv (
        const _TSCHAR *option
        )
{
        int retval;

        _mlock( _ENV_LOCK );

        __try {
            retval = _tputenv_helper(option, NULL);
        }
        __finally {
            _munlock( _ENV_LOCK );
        }

        return retval;
}


errno_t _tputenv_s (
        const _TSCHAR *name,
        const _TSCHAR *value
        )
{
        int retval;

        /* validation section */
        _VALIDATE_RETURN_ERRCODE(value != NULL, EINVAL);

        _mlock( _ENV_LOCK );

        __try {
            retval = ((_tputenv_helper(name, value) == 0) ? 0 : errno);
        }
        __finally {
            _munlock( _ENV_LOCK );
        }

        return retval;
}

static int __cdecl _tputenv_helper (
        const _TSCHAR *name,
        const _TSCHAR *value
        )
{
        _TSCHAR * newoption = NULL;
        size_t newoption_size = 0;

        /*
         * At startup, we obtain the 'native' flavor of environment strings
         * from the OS. So a "main" program has _environ and a "wmain" has
         * _wenviron loaded at startup. Only when the user gets or puts the
         * 'other' flavor do we convert it.
         */

        _VALIDATE_RETURN(name != NULL, EINVAL, -1);

        /* copy the new environent string */
        if (value == NULL)
        {
            const _TSCHAR *equal = _tcschr(name, _T('='));

            if (equal != NULL)
            {
                /* validate the length of both name and value */
                _VALIDATE_RETURN(equal - name < _MAX_ENV, EINVAL, -1);
                _VALIDATE_RETURN(_tcsnlen(equal + 1, _MAX_ENV) < _MAX_ENV, EINVAL, -1);
            }

            /* the string is already complete in name */
            newoption_size = _tcslen(name) + 1;
            if ((newoption = (_TSCHAR *)_calloc_crt(newoption_size, sizeof(_TSCHAR))) == NULL)
            {
                return -1;
            }

            _tcscpy_s(newoption, newoption_size, name);
        }
        else
        {
            size_t namelen = _tcsnlen(name, _MAX_ENV);
            size_t valuelen = _tcsnlen(value, _MAX_ENV);

            /* validate the length of both name and value */
            _VALIDATE_RETURN(namelen < _MAX_ENV, EINVAL, -1);
            _VALIDATE_RETURN(valuelen < _MAX_ENV, EINVAL, -1);

            /* we assemble the string from name and value (we assume _tcslen("=") == 1) */
            newoption_size = namelen + 1 + valuelen + 1;
            if ((newoption = (_TSCHAR *)_calloc_crt(newoption_size, sizeof(_TSCHAR))) == NULL)
            {
                return -1;
            }

            _tcscpy_s(newoption, newoption_size, name);
            newoption[namelen++] = _T('=');
            _tcscpy_s(newoption + namelen, newoption_size - namelen, value);
        }

#ifdef WPRFLAG
        if ( __crtwsetenv(&newoption, 1) != 0 )
        {
            /* if the set failed, we will free the option only if it was not consumed */
            if(newoption)
            {
                _free_crt(newoption);
                newoption=NULL;
            }
            return -1;
        }

        /* If other environment type exists, update it */
        if (_environ)
        {
            _setenvp();
        }
#else  /* WPRFLAG */
        /* Set requested environment type, primary call */
        if ( __crtsetenv(&newoption, 1) != 0 )
        {
            /* if the set failed, we will free the option only if it was not consumed */
            if(newoption)
            {
                _free_crt(newoption);
                newoption=NULL;
            }
            return -1;
        }

        /* If other environment type exists, update it */
        if (_wenviron)
        {
            _wsetenvp();
        }
#endif  /* WPRFLAG */

        return 0;
}

