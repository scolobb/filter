/*---------------------------------------------------------------------------*/
/*trace.h*/
/*---------------------------------------------------------------------------*/
/*The definitions for tracing the translator stack under ourselves.*/
/*---------------------------------------------------------------------------*/
/*Copyright (C) 2001, 2002, 2005, 2008 Free Software Foundation, Inc.
  Written by Sergiu Ivanov <unlimitedscolobb@gmail.com>.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or * (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
#define _GNU_SOURCE 1
/*---------------------------------------------------------------------------*/
#include <unistd.h>
#include <fcntl.h>
#include <hurd/fsys.h>
/*---------------------------------------------------------------------------*/
#include "debug.h"
#include "trace.h"
#include "node.h"
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*--------Functions----------------------------------------------------------*/
/*Traces the translator stack on the given underlying node until it
  finds the first translator called `name` and returns the port
  pointing to the translator sitting under this one.*/
error_t
  trace_find
  (mach_port_t underlying, const char *name, int flags, mach_port_t * port)
{
  error_t err = 0;

  /*Identity information about the current process */
  uid_t *uids;
  size_t nuids;

  gid_t *gids;
  size_t ngids;

  /*The name and arguments of the translator being passed now */
  char *argz = NULL;
  size_t argz_len = 0;

  /*The current working directory */
  char *cwd = NULL;

  /*The port to the directory containing the file pointed to by `underlying` */
  file_t dir;

  /*The unauthenticated version of `dir` */
  file_t unauth_dir;

  /*The control port of the current translator */
  fsys_t fsys;

  /*The port to the translator we are currently looking at */
  mach_port_t node = underlying;

  /*The port to the previous translator */
  mach_port_t prev_node = MACH_PORT_NULL;

  /*The retry name and retry type returned by fsys_getroot */
  string_t retry_name;
  retry_type retry;

  /*Finalizes the execution of this function */
  void finalize (void)
  {
    /*If there is a working directory, free it */
    if (cwd)
      free (cwd);

    /*If the ports to the directory exist */
    if (dir)
      PORT_DEALLOC (dir);
  }				/*finalize */

  /*Obtain the current working directory */
  cwd = getcwd (NULL, 0);
  if (!cwd)
    {
      LOG_MSG ("trace_find: Could not obtain cwd.");
      return EINVAL;
    }
  LOG_MSG ("trace_find: cwd: '%s'", cwd);

  /*Open a port to this directory */
  dir = file_name_lookup (cwd, 0, 0);
  if (!dir)
    {
      finalize ();
      return ENOENT;
    }

  /*Try to get the number of effective UIDs */
  nuids = geteuids (0, 0);
  if (nuids < 0)
    {
      finalize ();
      return EINVAL;
    }

  /*Allocate some memory for the UIDs on the stack */
  uids = alloca (nuids * sizeof (uid_t));

  /*Fetch the UIDs themselves */
  nuids = geteuids (nuids, uids);
  if (nuids < 0)
    {
      finalize ();
      return EINVAL;
    }

  /*Try to get the number of effective GIDs */
  ngids = getgroups (0, 0);
  if (ngids < 0)
    {
      finalize ();
      return EINVAL;
    }

  /*Allocate some memory for the GIDs on the stack */
  gids = alloca (ngids * sizeof (gid_t));

  /*Fetch the GIDs themselves */
  ngids = getgroups (ngids, gids);
  if (ngids < 0)
    {
      finalize ();
      return EINVAL;
    }

  /*Obtain the unauthenticated version of `dir` */
  err = io_restrict_auth (dir, &unauth_dir, 0, 0, 0, 0);
  if (err)
    {
      finalize ();
      return err;
    }

  char buf[256];
  char *_buf = buf;
  size_t len = 256;
  io_read (node, &_buf, &len, 0, len);
  LOG_MSG ("trace_find: Read from underlying: '%s'", buf);

  /*Go up the translator stack */
  for (; !err;)
    {
      /*retreive the name and options of the current translator */
      err = file_get_fs_options (node, &argz, &argz_len);
      if (err)
	break;

      LOG_MSG ("trace_find: Obtained translator '%s'", argz);
      if (strcmp (argz, name) == 0)
	{
	  LOG_MSG ("trace_find: Match. Stopping here.");
	  break;
	}

      /*try to fetch the control port for this translator */
      err = file_get_translator_cntl (node, &fsys);
      LOG_MSG ("trace_find: err = %d", (int) err);
      if (err)
	break;

      LOG_MSG ("trace_find: Translator control port: %lu",
	       (unsigned long) fsys);

      prev_node = node;

      /*fetch the root of the translator */
      err = fsys_getroot
	(fsys, unauth_dir, MACH_MSG_TYPE_COPY_SEND,
	 uids, nuids, gids, ngids,
	 flags | O_NOTRANS, &retry, retry_name, &node);

      LOG_MSG ("trace_find: fsys_getroot returned %d", (int) err);
      LOG_MSG ("trace_find: Translator root: %lu", (unsigned long) node);

      /*TODO: Remove this debug output. */
      /*char buf[256];
         char * _buf = buf;
         size_t len = 256;
         io_read(node, &_buf, &len, 0, len);
         LOG_MSG("trace_find: Read: '%s'", buf); */
    }

  /*If the error occurred (most probably) because of the fact that we
     have reached the top of the translator stack */
  if ((err == EMACH_SEND_INVALID_DEST) || (err == ENXIO))
    /*this is OK */
    err = 0;

  /*Return the port to read from */
  *port = prev_node;

  /*Return the result of operations */
  finalize ();
  return err;
}				/*trace_find */

/*---------------------------------------------------------------------------*/
