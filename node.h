/*---------------------------------------------------------------------------*/
/*node.h*/
/*---------------------------------------------------------------------------*/
/*Node management. Also see lnode.h*/
/*---------------------------------------------------------------------------*/
/*Based on the code of unionfs translator.*/
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
#ifndef __NODE_H__
#define __NODE_H__

/*---------------------------------------------------------------------------*/
#include <error.h>
#include <sys/stat.h>
#include <hurd/netfs.h>
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*--------Macros-------------------------------------------------------------*/
/*Checks whether the give node is the root of the filterfs filesystem*/
#define NODE_IS_ROOT(n) (((n)->nn->lnode->dir) ? (0) : (1))
/*---------------------------------------------------------------------------*/
/*Node flags*/
#define FLAG_NODE_ULFS_FIXED    0x00000001 /*this node should not be updated */
#define FLAG_NODE_INVALIDATE    0x00000002 /*this node must be updated */
#define FLAG_NODE_ULFS_UPTODATE	0x00000004 /*this node has just been updated */
/*---------------------------------------------------------------------------*/
/*The type of offset corresponding to the current platform*/
#ifdef __USE_FILE_OFFSET64
#	define OFFSET_T __off64_t
#else
#	define OFFSET_T __off_t
#endif /*__USE_FILE_OFFSET64*/
/*---------------------------------------------------------------------------*/
/*Deallocates the specified port*/
#define PORT_DEALLOC(p) (mach_port_deallocate(mach_task_self(), (p)))
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*The user-defined node for libnetfs*/
struct netnode
{
  /*the flags associated with this node (might be not required) */
  int flags;

  /*a port to the underlying filesystem */
  file_t port;
};				/*struct netnode */
/*---------------------------------------------------------------------------*/
typedef struct netnode netnode_t;
/*---------------------------------------------------------------------------*/
typedef struct node node_t;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*--------Global Variables---------------------------------------------------*/
/*The lock protecting the underlying filesystem*/
extern struct mutex ulfs_lock;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*--------Functions----------------------------------------------------------*/
/*Derives a new node from `lnode` and adds a reference to `lnode`*/
error_t node_create (node_t ** node);
/*---------------------------------------------------------------------------*/
/*Destroys the specified node and removes a light reference from the
  associated light node*/
void node_destroy (node_t * np);
/*---------------------------------------------------------------------------*/
/*Creates the root node and the corresponding lnode*/
error_t node_create_root (node_t ** root_node);
/*---------------------------------------------------------------------------*/
/*Initializes the port to the underlying filesystem for the root node*/
error_t node_init_root (mach_port_t underlying,	/*the port to the
						  underlying node */
			node_t * node	/*the root node */
			);
/*---------------------------------------------------------------------------*/
#endif /*__NODE_H__*/
