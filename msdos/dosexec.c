/********************************************
dosexec.c
copyright 2009,2010,2014, Thomas E. Dickey
copyright 1991-1994,1995, Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/*
 * $MawkId: dosexec.c,v 1.5 2014/09/14 22:29:49 tom Exp $
 *
 * @Log: dosexec.c,v @
 * Revision 1.3  1995/08/20  16:37:22  mike
 * exit(1) -> exit(2)
 *
 * Revision 1.2  1994/10/08  18:50:03  mike
 * remove SM_DOS
 *
 * Revision 1.1.1.1  1993/07/03  18:58:47  mike
 * move source to cvs
 *
 * Revision 1.4  1992/12/05  22:29:43  mike
 * dos patch 112d:
 * don't use string_buff
 * check COMSPEC
 *
 * Revision 1.3  1992/07/10  16:21:57  brennan
 * store exit code of input pipes
 *
 * Revision 1.2  1991/11/16  10:27:18  brennan
 * BINMODE
 *
 * Revision 1.1  91/10/29  09:45:56  brennan
 * Initial revision
 */

/* system() and pipes() for MSDOS and Win32 console */

#include "mawk.h"

#if defined(MSDOS) || defined(_WINNT)
#include "memory.h"
#include "files.h"
#include "fin.h"

#undef _POSIX_

#include <process.h>

#ifndef P_WAIT
#define P_WAIT _P_WAIT		/* works with VS2008 */
#endif

static char *my_shell;		/* e.g.   "c:\\sys\\command.com"  */
static char *command_opt;	/*  " /c"  */

static void
get_shell(void)
{
    char *s, *p;
    int len;

    if ((s = getenv("MAWKSHELL")) != 0) {
	/* break into shell part and option part */
	p = s;
	while (*p != ' ' && *p != '\t')
	    p++;
	len = p - s;
	my_shell = (char *) zmalloc(len + 1);
	memcpy(my_shell, s, len);
	my_shell[len] = 0;
	command_opt = p;
    } else if ((s = getenv("COMSPEC")) != 0) {
	my_shell = s;
	command_opt = " /c";
	/* leading space needed because of bug in command.com */
    } else {
	errmsg(0,
	       "cannot exec(), must set MAWKSHELL or COMSPEC in environment");
	exit(2);
    }
}

int
DOSexec(char *command)
{
    char xbuff[256];

    if (!my_shell)
	get_shell();

    sprintf(xbuff, "%s %s", command_opt, command);

    fflush(stderr);
    fflush(stdout);

    return spawnl(P_WAIT, my_shell, my_shell, xbuff, (char *) 0);
}

static int next_tmp;		/* index for naming temp files */
static char *tmpdir;		/* directory to hold temp files */
static unsigned mawkid;		/* unique to this mawk process */

/* compute the unique temp file name associated with id */
char *
tmp_file_name(int id,
	      char *buffer)
{
    if (mawkid == 0) {
	/* first time */
	union {
	    void *ptr;
	    unsigned w[2];
	} xptr;

	memset(&xptr, 0, sizeof(xptr));
	xptr.ptr = (void *) &mawkid;
	mawkid = xptr.w[1];

	tmpdir = getenv("MAWKTMPDIR");
	if (!tmpdir || strlen(tmpdir) > 80)
	    tmpdir = "";
    }

    (void) sprintf(buffer, "%sMAWK%04X.%03X", tmpdir, mawkid, id);
    return buffer;
}

/* open a pipe, returning a temp file identifier by
   reference
*/

PTR
get_pipe(char *command,
	 int type, int *tmp_idp)
{
    PTR retval;
    char xbuff[256];
    char *tmpname;

    *tmp_idp = next_tmp;
    tmpname = tmp_file_name(next_tmp, xbuff + 163);

    if (type == PIPE_OUT) {
	retval = (PTR) fopen(tmpname, (binmode() & 2) ? "wb" : "w");
    } else {
	sprintf(xbuff, "%s > %s", command, tmpname);
	tmp_idp[1] = DOSexec(xbuff);
	retval = (PTR) FINopen(tmpname, 0);
    }

    next_tmp++;
    return retval;
}

/* closing a fake pipes involves running the out pipe
   command
*/

int
close_fake_outpipe(char *command,
		   int tid)	/* identifies the temp file */
{
    char xbuff[256];
    char *tmpname = tmp_file_name(tid, xbuff + 163);
    int retval;

    sprintf(xbuff, "%s < %s", command, tmpname);
    retval = DOSexec(xbuff);
    (void) unlink(tmpname);
    return retval;
}

#endif /* MSDOS */
