/***
*getenv.c - get the value of an environment variable
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines getenv() - searches the environment for a string variable
*       and returns the value of it.
*
*******************************************************************************/

#include <sect_attribs.h>
#include <cruntime.h>
#include <internal.h>
#include <mtdll.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <awint.h>
#include <locale.h>
#include <dbgint.h>

errno_t __cdecl _wgetenv_s_helper(size_t *pReturnValue, wchar_t *buffer, size_t sizeInWords, const wchar_t *varname);
errno_t __cdecl _getenv_s_helper(size_t *pReturnValue, char *buffer, size_t sizeInBytes, const char *varname);

#ifdef _DEBUG
errno_t __cdecl _wdupenv_s_helper(wchar_t **pBuffer, size_t *pBufferSizeInWords, const wchar_t *varname,
    int nBlockUse, const char *szFileName, int nLine);
errno_t __cdecl _dupenv_s_helper(char **pBuffer, size_t *pBufferSizeInWords, const char *varname,
    int nBlockUse, const char *szFileName, int nLine);
#else  /* _DEBUG */
errno_t __cdecl _wdupenv_s_helper(wchar_t **pBuffer, size_t *pBufferSizeInWords, const wchar_t *varname);
errno_t __cdecl _dupenv_s_helper(char **pBuffer, size_t *pBufferSizeInWords, const char *varname);
#endif  /* _DEBUG */

#ifdef WPRFLAG
#define _tgetenv_helper_nolock _wgetenv_helper_nolock
#define _tgetenv_s_helper _wgetenv_s_helper
#define _tdupenv_s_helper _wdupenv_s_helper
#define _tsetenvp    _wsetenvp
#define _tenviron    _wenviron
#else  /* WPRFLAG */
#define _tgetenv_helper_nolock _getenv_helper_nolock
#define _tgetenv_s_helper _getenv_s_helper
#define _tdupenv_s_helper _dupenv_s_helper
#define _tsetenvp    _setenvp

#define _tenviron  _environ

#endif  /* WPRFLAG */

/***
*char *getenv(option) - search environment for a string
*
*Purpose:
*       searches the environment for a string of the form "option=value",
*       if found, return value, otherwise NULL.
*
*Entry:
*       const char *option - variable to search for in environment
*
*Exit:
*       returns the value part of the environment string if found,
*       otherwise NULL
*
*Exceptions:
*
*******************************************************************************/

_TSCHAR * __cdecl _tgetenv (
        const _TSCHAR *option
        )
{
        _TSCHAR *retval;

        _VALIDATE_RETURN( (option != NULL), EINVAL, NULL);
        _VALIDATE_RETURN( (_tcsnlen(option, _MAX_ENV) < _MAX_ENV), EINVAL, NULL);

        _mlock( _ENV_LOCK );
        __try {
            retval = (_TSCHAR *)_tgetenv_helper_nolock(option);
        }
        __finally {
            _munlock( _ENV_LOCK );
        }

        return(retval);
}

const _TSCHAR * __cdecl _tgetenv_helper_nolock (
        const _TSCHAR *option
        )
{
        _TSCHAR **search;
        _TSCHAR *retValue = NULL;
        size_t length;
        BOOL firstSearch = TRUE;
        BOOL searchEnv = TRUE;
#ifndef WPRFLAG
        _TSCHAR *upperCaseOption = NULL;
        _TSCHAR *upperCaseSearch = NULL;
#endif  /* WPRFLAG */

        if (!option)
            return NULL;

        /*
         * At startup, we obtain the 'native' flavor of environment strings
         * from the OS. So a "main" program has _environ and a "wmain" has
         * _wenviron loaded at startup. Only when the user gets or puts the
         * 'other' flavor do we convert it.
         */

        if (!_tenviron)
        {
            if ( _tsetenvp() < 0 || !_tenviron)
            {
                return NULL;
            }
        }

        length = _tcslen(option);

#ifndef WPRFLAG
        if ((upperCaseOption = (_TSCHAR *)_calloc_crt(length + 1, sizeof(_TSCHAR))) == NULL)
        {
            return NULL;
        }

        /* Convert to upper case for quick comparison */
        LCMapStringA(LOCALE_INVARIANT, LCMAP_UPPERCASE, option, (int)length, upperCaseOption, (int)length + 1);
#endif  /* WPRFLAG */

        while (searchEnv)
        {
            searchEnv = FALSE;

            /* Initialize search to point to the environment block */
            search = _tenviron;

            /*
            ** Make sure `*search' is long enough to be a candidate
            ** (We must NOT index past the '\0' at the end of `*search'!)
            ** and that it has an equal sign ('=') in the correct spot.
            ** If both of these requirements are met, compare the strings.
            */
            while (*search)
            {
                if (_tcslen(*search) > length &&
                    (*(*search + length) == _T('=')) &&
#ifdef WPRFLAG
                    (_tcsnicoll(*search, option, length) == 0))
#else  /* WPRFLAG */
                   (upperCaseSearch = (_TSCHAR *)_calloc_crt(length + 1, sizeof(_TSCHAR))) &&
                   LCMapStringA(LOCALE_INVARIANT, LCMAP_UPPERCASE, *search, (int)length, upperCaseSearch, (int)length + 1) &&
                   (strncmp(upperCaseOption, upperCaseSearch, length) == 0))
#endif  /* WPRFLAG */
                {
                    /* internal consistency check: the environment string should never use a buffer bigger than _MAX_ENV
                         * see also SDK function SetEnvironmentVariable
                         */
                    _ASSERTE(_tcsnlen(*search + length + 1, _MAX_ENV) < _MAX_ENV);

                    /* save the result */
                    retValue = *search + length + 1;
                }

#ifndef WPRFLAG
                /* Don't leak memory allocated for search env variable name per iteration */
                _free_crt(upperCaseSearch);
                upperCaseSearch = NULL;
#endif  /* WPRFLAG */
                search++;
            }

            /* At this point we haven't been able to find the environment variable,
            ** so try the O/S.
            */
            if (retValue == NULL && firstSearch)
            {
                /* Query the O/S for the environment variable only once */
                firstSearch = FALSE;

                /* Query the environment variable from Windows. */
                if (GetEnvironmentVariable(option, NULL, 0) != 0 &&
                     _tsetenvp() < 0 )
                {
                    searchEnv = TRUE; /* enable searching one more time */
                }
            }
        }

#ifndef WPRFLAG
        _free_crt(upperCaseOption);
#endif  /* WPRFLAG */
        return retValue;
}

/***
*errno_t getenv_s(pReturnValue, buffer, size, option) - search environment for a string
*
*Purpose:
*       searches the environment for a string of the form "option=value",
*       if found, copies the value in buffer.
*
*Entry:
*       size_t *pReturnValue - indicates if the variable has been found and
*           size needed
*       char *buffer - destination string
*       size_t sizeInChars - size of the destination buffer
*       const char *varname - variable to search for in environment
*
*Exit:
*       return the error code
*
*Exceptions:
*
*******************************************************************************/

errno_t __cdecl _tgetenv_s (
        size_t *pReturnValue,
        _TSCHAR *buffer,
        size_t sizeInTChars,
        const _TSCHAR *varname
        )
{
        errno_t retval;

        _mlock( _ENV_LOCK );
        __try {
            retval = _tgetenv_s_helper(pReturnValue, buffer, sizeInTChars, varname);
        }
        __finally {
            _munlock( _ENV_LOCK );
        }

        return(retval);
}

static
__forceinline
errno_t __cdecl _tgetenv_s_helper (
        size_t *pReturnValue,
        _TSCHAR *buffer,
        size_t sizeInTChars,
        const _TSCHAR *varname
        )
{
    const _TSCHAR *str;

    /* validation section */
    _VALIDATE_RETURN_ERRCODE(pReturnValue != NULL, EINVAL);
    *pReturnValue = 0;
    _VALIDATE_RETURN_ERRCODE((buffer != NULL && sizeInTChars > 0) || (buffer == NULL && sizeInTChars == 0), EINVAL);
    if (buffer != NULL)
    {
        *buffer = '\0';
    }

    /* varname is already validated in _tgetenv_helper_nolock */
    str = _tgetenv_helper_nolock(varname);
    if (str == NULL)
    {
        return 0;
    }

    *pReturnValue = _tcslen(str) + 1;
    if (sizeInTChars == 0)
    {
        /* we just return the size of the needed buffer */
        return 0;
    }

    if (*pReturnValue > sizeInTChars)
    {
        /* the buffer is too small: we return EINVAL, and we give the user another chance to
         * call getenv_s with a bigger buffer
         */
        return ERANGE;
    }

    _ERRCHECK(_tcscpy_s(buffer, sizeInTChars, str));

    return 0;
}

/***
*errno_t _dupenv_s(pBuffer, pBufferSize, varname) - search environment for a string
*
*Purpose:
*       searches the environment for a string of the form "option=value",
*       if found, copies the value in buffer.
*
*Entry:
*       char **pBuffer - pointer to a buffer which will be allocated
*       size_t *pBufferSize - pointer to the size of the buffer (optional)
*       const char *varname - variable to search for in environment
*
*Exit:
*       return the error code
*
*Exceptions:
*
*******************************************************************************/

#ifdef _DEBUG
errno_t __cdecl _tdupenv_s (
        _TSCHAR **pBuffer,
        size_t *pBufferSizeInTChars,
        const _TSCHAR *varname
        )
{
    return _tdupenv_s_dbg(pBuffer, pBufferSizeInTChars, varname, _NORMAL_BLOCK, NULL, 0);
}

errno_t __cdecl _tdupenv_s_dbg (
        _TSCHAR **pBuffer,
        size_t *pBufferSizeInTChars,
        const _TSCHAR *varname,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
#else  /* _DEBUG */
errno_t __cdecl _tdupenv_s (
        _TSCHAR **pBuffer,
        size_t *pBufferSizeInTChars,
        const _TSCHAR *varname
        )
#endif  /* _DEBUG */
{
        errno_t retval;

        _mlock( _ENV_LOCK );
        __try {
#ifdef _DEBUG
            retval = _tdupenv_s_helper(pBuffer, pBufferSizeInTChars, varname, nBlockUse, szFileName, nLine);
#else  /* _DEBUG */
            retval = _tdupenv_s_helper(pBuffer, pBufferSizeInTChars, varname);
#endif  /* _DEBUG */
        }
        __finally {
            _munlock( _ENV_LOCK );
        }

        return retval;
}

static
__forceinline
#ifdef _DEBUG
errno_t __cdecl _tdupenv_s_helper (
        _TSCHAR **pBuffer,
        size_t *pBufferSizeInTChars,
        const _TSCHAR *varname,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
#else  /* _DEBUG */
errno_t __cdecl _tdupenv_s_helper (
        _TSCHAR **pBuffer,
        size_t *pBufferSizeInTChars,
        const _TSCHAR *varname
        )
#endif  /* _DEBUG */
{
    const _TSCHAR *str;
    size_t size;

    /* validation section */
    _VALIDATE_RETURN_ERRCODE(pBuffer != NULL, EINVAL);
    *pBuffer = NULL;
    if (pBufferSizeInTChars != NULL)
    {
        *pBufferSizeInTChars = 0;
    }
    _VALIDATE_RETURN_ERRCODE(varname != NULL, EINVAL);

    str = _tgetenv_helper_nolock(varname);
    if (str == NULL)
    {
        return 0;
    }

    size = _tcslen(str) + 1;
#ifdef _DEBUG
    *pBuffer = (_TSCHAR*)_calloc_dbg(size, sizeof(_TSCHAR), nBlockUse, szFileName, nLine);
#else  /* _DEBUG */
    *pBuffer = (_TSCHAR*)calloc(size, sizeof(_TSCHAR));
#endif  /* _DEBUG */
    if (*pBuffer == NULL)
    {
        errno = ENOMEM;
        return errno;
    }

    _ERRCHECK(_tcscpy_s(*pBuffer, size, str));
    if (pBufferSizeInTChars != NULL)
    {
        *pBufferSizeInTChars = size;
    }
    return 0;
}
