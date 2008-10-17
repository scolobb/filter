/*----------------------------------------------------------------------------*/
/*node.c*/
/*----------------------------------------------------------------------------*/
/*Implementation of node management strategies*/
/*----------------------------------------------------------------------------*/
/*Based on the code of unionfs translator.*/
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
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
/*----------------------------------------------------------------------------*/
#include "debug.h"
#include "node.h"
#include "filter.h"
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--------Global Variables----------------------------------------------------*/
/*The lock protecting the underlying filesystem*/
struct mutex ulfs_lock = MUTEX_INITIALIZER;
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--------Functions-----------------------------------------------------------*/
/*Derives a new node from `lnode` and adds a reference to `lnode`*/
error_t
node_create
	(
	node_t ** node	/*store the result here*/
	)
	{
	error_t err = 0;

	/*Create a new netnode*/
	netnode_t * netnode_new = malloc(sizeof(netnode_t));
	
	/*If the memory could not be allocated*/
	if(netnode_new == NULL)
		err = ENOMEM;
	else
		{
		/*create a new node from the netnode*/
		node_t * node_new = netfs_make_node(netnode_new);
		
		/*If the creation failed*/
		if(node_new == NULL)
			{
			/*set the error code*/
			err = ENOMEM;

			/*destroy the netnode created above*/
			free(netnode_new);
			
			/*stop*/
			return err;
			}
		
		/*store the result of creation in the second parameter*/
		*node = node_new;
		}
		
	/*Return the result of operations*/
	return err;
	}/*node_create*/
/*----------------------------------------------------------------------------*/
/*Destroys the specified node and removes a light reference from the
	associated light node*/
void
node_destroy
	(
	node_t * np
	)
	{
	/*Destroy the port to the underlying filesystem allocated to the node*/
	PORT_DEALLOC(np->nn->port);
	
	/*Free the netnode and the node itself*/
	free(np->nn);
	free(np);
	}/*node_destroy*/
/*----------------------------------------------------------------------------*/
/*Creates the root node and the corresponding lnode*/
error_t
node_create_root
	(
	node_t ** root_node	/*store the result here*/
	)
	{
	error_t err;

	/*Try to create a node*/
	node_t * node;
	err = node_create(&node);
	if(err)
		return err;
		
	/*Store the result in the parameter*/
	*root_node = node;
	
	/*Return the result*/
	return err;
	}/*node_create_root*/
/*----------------------------------------------------------------------------*/
/*Initializes the port to the underlying filesystem for the root node*/
error_t
node_init_root
	(
	mach_port_t underlying,	/*the port to the underlying node*/
	node_t * node						/*the root node*/
	)
	{
	error_t err = 0;

	/*Acquire a lock for operations on the underlying filesystem*/
	mutex_lock(&ulfs_lock);
	
	/*Store the specified port in the node*/
	node->nn->port = underlying;
	
	LOG_MSG("node_init_root: Port: 0x%ld", (unsigned long)node->nn->port);
	
	/*Stat the root node*/
	err = io_stat(node->nn->port, &node->nn_stat);
	if(err)
		{
		/*deallocate the port*/
		PORT_DEALLOC(node->nn->port);
		
		LOG_MSG("node_init_root: Could not stat the root node.");
		
		/*unlock the mutex and exit*/
		mutex_unlock(&ulfs_lock);
		return err;
		}

	/*Release the lock for operations on the undelying filesystem*/
	mutex_unlock(&ulfs_lock);
	
	/*Return the result of operations*/
	return err;
	}/*node_init_root*/
/*----------------------------------------------------------------------------*/
