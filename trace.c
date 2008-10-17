/*----------------------------------------------------------------------------*/
/*trace.h*/
/*----------------------------------------------------------------------------*/
/*The definitions for tracing the translator stack under ourselves.*/
/*----------------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
#define _GNU_SOURCE 1
/*----------------------------------------------------------------------------*/
#include <unistd.h>
#include <fcntl.h>
#include <hurd/fsys.h>
/*----------------------------------------------------------------------------*/
#include "debug.h"
#include "trace.h"
#include "node.h"
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--------Functions-----------------------------------------------------------*/
/*Traces the translator stack on the given underlying node until it finds the
	first translator called `name` and returns the port pointing to the
	translator sitting under this one.*/
error_t
trace_find
	(
	mach_port_t underlying,
	const char * name,
	int flags,
	mach_port_t * port
	)
	{
	}/*trace_find*/
/*----------------------------------------------------------------------------*/
