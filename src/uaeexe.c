/*
 *  uaeexe.c - UAE remote cli
 *
 *  (c) 1997 by Samuel Devulder
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "uaeexe.h"

static struct uae_xcmd *first = NULL;
static struct uae_xcmd *last  = NULL;
<<<<<<< HEAD
static char running = 0;
=======
static TCHAR running = 0;
>>>>>>> p-uae/v2.1.0
static uae_u32 uaeexe_server (TrapContext *context) REGPARAM;

/*
 * Install the server
 */
void uaeexe_install (void)
{
    uaecptr loop;

    loop = here ();
    org (UAEEXE_ORG);
<<<<<<< HEAD
    calltrap (deftrap (uaeexe_server));
=======
	calltrap (deftrapres (uaeexe_server, 0, "uaeexe_server"));
>>>>>>> p-uae/v2.1.0
    dw (RTS);
    org (loop);
}

/*
 * Send command to the remote cli.
 *
 * To use this, just call uaeexe("command") and the command will be
 * executed by the remote cli (provided you've started it in the
 * s:user-startup for example). Be sure to add "run" if you want
 * to launch the command asynchronously. Please note also that the
 * remote cli works better if you've got the fifo-handler installed.
 */
<<<<<<< HEAD
int uaeexe (const char *cmd)
=======
int uaeexe (const TCHAR *cmd)
>>>>>>> p-uae/v2.1.0
{
    struct uae_xcmd *nw;

    if (!running)
<<<<<<< HEAD
	goto NORUN;

    nw = (struct uae_xcmd *) malloc (sizeof *nw);
    if (!nw)
	goto NOMEM;
    nw->cmd = (char *) malloc (strlen (cmd) + 1);
    if (!nw->cmd) {
	free (nw);
	goto NOMEM;
    }

    strcpy (nw->cmd, cmd);
=======
		goto NORUN;

	nw = xmalloc (struct uae_xcmd, 1);
    if (!nw)
		goto NOMEM;
	nw->cmd = xmalloc (TCHAR, _tcslen (cmd) + 1);
    if (!nw->cmd) {
		xfree (nw);
		goto NOMEM;
    }

	_tcscpy (nw->cmd, cmd);
>>>>>>> p-uae/v2.1.0
    nw->prev = last;
    nw->next = NULL;

    if (!first)
<<<<<<< HEAD
	first      = nw;
    if (last) {
	last->next = nw;
	last       = nw;
    } else
	last       = nw;

    return UAEEXE_OK;
  NOMEM:
    return UAEEXE_NOMEM;
  NORUN:
    write_log ("Remote cli is not running.\n");
=======
		first      = nw;
    if (last) {
		last->next = nw;
		last       = nw;
    } else
		last       = nw;

	return UAEEXE_OK;
NOMEM:
    return UAEEXE_NOMEM;
NORUN:
	write_log ("Remote cli is not running.\n");
>>>>>>> p-uae/v2.1.0
    return UAEEXE_NOTRUNNING;
}

/*
 * returns next command to be executed
 */
<<<<<<< HEAD
static char *get_cmd (void)
{
    struct uae_xcmd *cmd;
    char *s;

    if (!first)
	return NULL;
=======
static TCHAR *get_cmd (void)
{
    struct uae_xcmd *cmd;
	TCHAR *s;

    if (!first)
		return NULL;
>>>>>>> p-uae/v2.1.0
    s = first->cmd;
    cmd = first;
    first = first->next;
    if (!first)
<<<<<<< HEAD
	last = NULL;
=======
		last = NULL;
>>>>>>> p-uae/v2.1.0
    free (cmd);
    return s;
}

/*
 * helper function
 */
<<<<<<< HEAD
#define ARG(x) (get_long (m68k_areg (&context->regs, 7) + 4*(x+1)))
static uae_u32 REGPARAM2 uaeexe_server (TrapContext *context)
{
    int len;
    char *cmd;
    char *dst;

    if (ARG(0) && !running) {
	running = 1;
	write_log ("Remote CLI started.\n");
=======
#define ARG(x) (get_long (m68k_areg (regs, 7) + 4 * (x + 1)))
static uae_u32 REGPARAM2 uaeexe_server (TrapContext *context)
{
    int len;
	TCHAR *cmd;
	char *dst;

    if (ARG(0) && !running) {
		running = 1;
		write_log ("Remote CLI started.\n");
>>>>>>> p-uae/v2.1.0
    }

    cmd = get_cmd ();
    if (!cmd)
<<<<<<< HEAD
	return 0;
    if (!ARG(0)) {
	running = 0;
	return 0;
=======
		return 0;
    if (!ARG(0)) {
		running = 0;
		return 0;
>>>>>>> p-uae/v2.1.0
    }

    dst = (char *) get_real_address (ARG (0));
    len = ARG (1);
<<<<<<< HEAD
    strncpy (dst, cmd, len);
    write_log ("Sending '%s' to remote cli\n", cmd);
    free (cmd);
=======
	strncpy (dst, cmd, len);
	write_log ("Sending '%s' to remote cli\n", cmd);
	xfree (cmd);
>>>>>>> p-uae/v2.1.0
    return ARG (0);
}
