/** 
 * @namespace   libbeye
 * @file        libbeye/osdep/unix/os_dep.c
 * @brief       This file contains implementation of unix compatible OS dependent part.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Konstantin Boldyshev
 * @since       1999
 * @note        Development, fixes and improvements
 *
 * @author      Mauro Giachero
 * @since       11.2007
 * @note        Added __get_home_dir() and some optimizations
**/

/*
    Copyright (C) 1999-2002 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id: os_dep.c,v 1.10 2009/09/03 16:57:40 nickols_k Exp $
*/

#ifndef lint
static const char rcs_id[] = "$Id: os_dep.c,v 1.10 2009/09/03 16:57:40 nickols_k Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "libbeye/libbeye.h"
#include "console.h"

#ifndef	PREFIX
#define	PREFIX	"/usr/local"
#endif

#ifndef	DATADIR
#define DATADIR	PREFIX"/share/beye"
#endif

tBool break_status = False;	/**< CTRL+BREAK flag */

termdesc *terminal = NULL;

static termdesc termtab[] = {
{ "linux",	TERM_LINUX },
{ "console",	TERM_LINUX },
{ "xterm",	TERM_XTERM },
{ "xterm-color", TERM_XTERM},
{ "color-xterm", TERM_XTERM},
{ "beterm",	TERM_XTERM },
{ "vt100",	TERM_VT100 },
{ "ansi",	TERM_ANSI  },
{ NULL,		TERM_UNKNOWN}};

static char _ini_name[FILENAME_MAX + 1];
static char _rc_dir_name[FILENAME_MAX + 1];
static char _home_dir_name[FILENAME_MAX + 1];

/*
The home directory is a good place for configuration
and temporary files.
At least (strlen(progname) + 9) characters should be
available before the buffer end.
The trailing '/' is included in the returned string.
*/
char * __FASTCALL__ __get_home_dir(const char *progname)
{
    char *p;

    if (_home_dir_name[0]) return _home_dir_name; //Already computed

    p = getenv("HOME");
    if (p == NULL || strlen(p) < 2) {
	struct passwd *psw = getpwuid(getuid());
	if (psw != NULL) p = psw->pw_dir;
    }

    if (p == NULL || strlen(p) > FILENAME_MAX - (strlen(progname) + 10))
	p = "/tmp";

    strcpy(_home_dir_name, p);
    strcat(_home_dir_name, "/");

    return _home_dir_name;
}

/*

*/

char * __FASTCALL__ __get_ini_name(const char *progname)
{
    char *p;

    if (_ini_name[0]) return _ini_name; //Already computed

    p = __get_home_dir(progname);
    strcpy(_ini_name, p);
    strcat(_ini_name, ".");
    strcat(_ini_name, progname);
    return strcat(_ini_name, "rc");
}

char * __FASTCALL__ __get_rc_dir(const char *progname)
{
    if (_rc_dir_name[0]) return _rc_dir_name; //Already computed

    strcpy(_rc_dir_name, DATADIR);
    /*strcat(_rc_dir_name, progname);*/
    return strcat(_rc_dir_name, "/");
}


tBool __FASTCALL__ __OsGetCBreak(void)
{
#ifndef	__ENABLE_SIGIO
    ReadNextEvent();
#endif
    return break_status;
}

void  __FASTCALL__ __OsSetCBreak(tBool state)
{
    break_status = state;
}

void __FASTCALL__ __OsYield(void)
{
#ifdef	__BEOS__
    /* usleep(10000); */
#else
    struct timespec t = { 0, 100000 };
    nanosleep(&t, NULL);
#endif
}

static void cleanup(int sig)
{
    __term_keyboard();
    __term_vio();
    __term_sys();
    printm("Terminated by signal %d\n", sig);
    _exit(EXIT_FAILURE);
}

/* static struct sigaction sa; */

void __FASTCALL__ __init_sys(void)
{
    int i = 0;
    char *t = getenv("TERM");

    if (t != NULL)
	for (i = 0; termtab[i].name != NULL && strcasecmp(t, termtab[i].name); i++);
    terminal = &termtab[i];

    if (terminal->type == TERM_UNKNOWN) {
        printm("Sorry, I can't handle terminal type '%s'.\nIf you are sure it is vt100 compatible, try setting TERM to vt100:\n\n$ TERM=vt100 beye filename\n", t);
        exit(2);
    }
	
    if (i == 5) output_7 = 1;	/* beterm is (B)roken (E)vil (TERM)inal */

    if (terminal->type == TERM_XTERM) {
	t = getenv("COLORTERM");
	if (t != NULL && !strcasecmp(t, "Eterm")) transparent = 1;
    }

    _ini_name[0] = '\0';
    _rc_dir_name[0] = '\0';
    _home_dir_name[0] = '\0';

    umask(0077);
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);
    signal(SIGQUIT, cleanup);
    signal(SIGILL, cleanup);
/*
    sa.sa_handler = cleanup;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
*/
}

void __FASTCALL__ __term_sys(void)
{
}

