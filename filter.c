/*----------------------------------------------------------------------------*/
/*filter.c*/
/*----------------------------------------------------------------------------*/
/*The filtering translator*/
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
#include "filter.h"
/*----------------------------------------------------------------------------*/
#include <error.h>
#include <argp.h>
#include <argz.h>
#include <hurd/netfs.h>
#include <fcntl.h>
/*----------------------------------------------------------------------------*/
#include "debug.h"
#include "options.h"
#include "trace.h"
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--------Global Variables----------------------------------------------------*/
/*The name of the server*/
char * netfs_server_name = "filter";
/*----------------------------------------------------------------------------*/
/*The version of the server*/
char * netfs_server_version = "0.0";
/*----------------------------------------------------------------------------*/
/*The maximal length of a chain of symbolic links*/
int netfs_maxsymlinks = 12;
/*----------------------------------------------------------------------------*/
/*A port to the underlying node*/
mach_port_t underlying_node;
/*----------------------------------------------------------------------------*/
/*Status information for the underlying node*/
io_statbuf_t underlying_node_stat;
/*----------------------------------------------------------------------------*/
/*Mapped time used for updating node information*/
volatile struct mapped_time_value * maptime;
/*----------------------------------------------------------------------------*/
/*The filesystem ID*/
pid_t fsid;
/*----------------------------------------------------------------------------*/
/*The port from which we will read (TODO: and write) the data*/
mach_port_t target;
/*----------------------------------------------------------------------------*/
/*The file to print debug messages to*/
FILE * filter_dbg;
/*----------------------------------------------------------------------------*/
/*The name of the translator to filter out*/
char * target_name = NULL;
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--------Functions-----------------------------------------------------------*/
/*Attempts to create a file named `name` in `dir` for `user` with mode `mode`*/
error_t
netfs_attempt_create_file
	(
	struct iouser * user,
	struct node * dir,
	char * name,
	mode_t mode,
	struct node ** node
	)
	{
	LOG_MSG("netfs_attempt_create_file");

	/*Unlock `dir` and say that we can do nothing else here*/
	mutex_unlock(&dir->lock);
	return EOPNOTSUPP;
	}/*netfs_attempt_create_file*/
/*----------------------------------------------------------------------------*/
/*Return an error if the process of opening a file should not be allowed
	to complete because of insufficient permissions*/
error_t
netfs_check_open_permissions
	(
	struct iouser * user,
	struct node * np,
	int flags,
	int newnode
	)
	{
	LOG_MSG("netfs_check_open_permissions");

	error_t err = 0;
	
	/*Cheks user's permissions*/
	if(flags & O_READ)
		err = fshelp_access(&np->nn_stat, S_IREAD, user);
	if(!err && (flags & O_WRITE))
		err = fshelp_access(&np->nn_stat, S_IWRITE, user);
	if(!err && (flags & O_EXEC))
		err = fshelp_access(&np->nn_stat, S_IEXEC, user);
		
	/*Return the result of the check*/
	return err;
	}/*netfs_check_open_permissions*/
/*----------------------------------------------------------------------------*/
/*Attempts an utimes call for the user `cred` on node `node`*/
error_t
netfs_attempt_utimes
	(
	struct iouser * cred,
	struct node * node,
	struct timespec * atime,
	struct timespec * mtime
	)
	{
	LOG_MSG("netfs_attempt_utimes");

	error_t err = 0;
	
	/*See what information is to be updated*/
	int flags = TOUCH_CTIME;
	
	/*Check if the user is indeed the owner of the node*/
	err = fshelp_isowner(&node->nn_stat, cred);
	
	/*If the user is allowed to do utimes*/
	if(!err)
		{
		/*If atime is to be updated*/
		if(atime)
			/*update the atime*/
			node->nn_stat.st_atim = *atime;
		else
			/*the current time will be set as the atime*/
			flags |= TOUCH_ATIME;
		
		/*If mtime is to be updated*/
		if(mtime)
			/*update the mtime*/
			node->nn_stat.st_mtim = *mtime;
		else
			/*the current time will be set as mtime*/
			flags |= TOUCH_MTIME;
		
		/*touch the file*/
		fshelp_touch(&node->nn_stat, flags, maptime);
		}
	
	/*Return the result of operations*/
	return err;
	}/*netfs_attempt_utimes*/
/*----------------------------------------------------------------------------*/
/*Returns the valid access types for file `node` and user `cred`*/
error_t
netfs_report_access
	(
	struct iouser * cred,
	struct node * np,
	int * types
	)
	{
	LOG_MSG("netfs_report_access");

	/*No access at first*/
	*types = 0;
	
	/*Check the access and set the required bits*/
	if(fshelp_access(&np->nn_stat, S_IREAD,  cred) == 0)
		*types |= O_READ;
	if(fshelp_access(&np->nn_stat, S_IWRITE, cred) == 0)
		*types |= O_WRITE;
	if(fshelp_access(&np->nn_stat, S_IEXEC,  cred) == 0)
		*types |= O_EXEC;

	/*Everything OK*/
	return 0;
	}/*netfs_report_access*/
/*----------------------------------------------------------------------------*/
/*Validates the stat data for the node*/
error_t
netfs_validate_stat
	(
	struct node * np,
	struct iouser * cred
	)
	{
	LOG_MSG("netfs_validate_stat");

	error_t err = 0;
	
	/*Validate the stat information about the node*/
	err = io_stat(np->nn->port, &np->nn_stat);

	/*Return the result of operations*/
	return err;
	}/*netfs_validate_stat*/
/*----------------------------------------------------------------------------*/
/*Syncs `node` completely to disk*/
error_t
netfs_attempt_sync
	(
	struct iouser * cred,
	struct node * node,
	int wait
	)
	{
	LOG_MSG("netfs_attempt_sync");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_sync*/
/*----------------------------------------------------------------------------*/
/*Fetches a directory*/
error_t
netfs_get_dirents
	(
	struct iouser * cred,
	struct node * dir,
	int first_entry,
	int num_entries,
	char ** data,
	mach_msg_type_number_t * data_len,
	vm_size_t max_data_len,
	int * data_entries
	)
	{
	LOG_MSG("netfs_get_dirents");
	
	/*This node is not a directory*/
	return ENOTDIR;
	}/*netfs_get_dirents*/
/*----------------------------------------------------------------------------*/
/*Looks up `name` under `dir` for `user`*/
error_t
netfs_attempt_lookup
	(
	struct iouser * user,
	struct node * dir,
	char * name,
	struct node ** node
	)
	{
	LOG_MSG("netfs_attempt_lookup: '%s'", name);
		
	/*Unlock the mutexes in `dir`*/
	mutex_unlock(&dir->lock);
	return EOPNOTSUPP;
	}/*netfs_attempt_lookup*/
/*----------------------------------------------------------------------------*/
/*Deletes `name` in `dir` for `user`*/
error_t
netfs_attempt_unlink
	(
	struct iouser * user,
	struct node * dir,
	char * name
	)
	{
	LOG_MSG("netfs_attempt_unlink");

	return 0;
	}/*netfs_attempt_unlink*/
/*----------------------------------------------------------------------------*/
/*Attempts to rename `fromdir`/`fromname` to `todir`/`toname`*/
error_t
netfs_attempt_rename
	(
	struct iouser * user,
	struct node * fromdir,
	char * fromname,
	struct node * todir,
	char * toname,
	int excl
	)
	{
	LOG_MSG("netfs_attempt_rename");

	/*Operation not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_rename*/
/*----------------------------------------------------------------------------*/
/*Attempts to create a new directory*/
error_t
netfs_attempt_mkdir
	(
	struct iouser * user,
	struct node * dir,
	char * name,
	mode_t mode
	)
	{
	LOG_MSG("netfs_attempt_mkdir");

	return 0;
	}/*netfs_attempt_mkdir*/
/*----------------------------------------------------------------------------*/
/*Attempts to remove directory `name` in `dir` for `user`*/
error_t
netfs_attempt_rmdir
	(
	struct iouser * user,
	struct node * dir,
	char * name
	)
	{
	LOG_MSG("netfs_attempt_rmdir");

	return 0;
	}/*netfs_attempt_rmdir*/
/*----------------------------------------------------------------------------*/
/*Attempts to change the mode of `node` for user `cred` to `uid`:`gid`*/
error_t
netfs_attempt_chown
	(
	struct iouser * cred,
	struct node * node,
	uid_t uid,
	uid_t gid
	)
	{
	LOG_MSG("netfs_attempt_chown");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_chown*/
/*----------------------------------------------------------------------------*/
/*Attempts to change the author of `node` to `author`*/
error_t
netfs_attempt_chauthor
	(
	struct iouser * cred,
	struct node * node,
	uid_t author
	)
	{
	LOG_MSG("netfs_attempt_chauthor");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_chauthor*/
/*----------------------------------------------------------------------------*/
/*Attempts to change the mode of `node` to `mode` for `cred`*/
error_t
netfs_attempt_chmod
	(
	struct iouser * user,
	struct node * node,
	mode_t mode
	)
	{
	LOG_MSG("netfs_attempt_chmod");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_chmod*/
/*----------------------------------------------------------------------------*/
/*Attempts to turn `node` into a symlink targetting `name`*/
error_t
netfs_attempt_mksymlink
	(
	struct iouser * cred,
	struct node * node,
	char * name
	)
	{
	LOG_MSG("netfs_attempt_mksymlink");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_mksymlink*/
/*----------------------------------------------------------------------------*/
/*Attempts to turn `node` into a device; type can be either S_IFBLK or S_IFCHR*/
error_t
netfs_attempt_mkdev
	(
	struct iouser * cred,
	struct node * node,
	mode_t type,
	dev_t indexes
	)
	{
	LOG_MSG("netfs_attempt_mkdev");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_mkdev*/
/*----------------------------------------------------------------------------*/
/*Attempts to set the passive translator record for `file` passing `argz`*/
error_t
netfs_set_translator
	(
	struct iouser * cred,
	struct node * node,
	char * argz,
	size_t arglen
	)
	{
	LOG_MSG("netfs_set_translator");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_set_translator	*/
/*----------------------------------------------------------------------------*/
/*Attempts to call chflags for `node`*/
error_t
netfs_attempt_chflags
	(
	struct iouser * cred,
	struct node * node,
	int flags
	)
	{
	LOG_MSG("netfs_attempt_chflags");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_chflags*/
/*----------------------------------------------------------------------------*/
/*Attempts to set the size of file `node`*/
error_t
netfs_attempt_set_size
	(
	struct iouser * cred,
	struct node * node,
	loff_t size
	)
	{
	LOG_MSG("netfs_attempt_set_size");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_set_size*/
/*----------------------------------------------------------------------------*/
/*Fetches the filesystem status information*/
error_t
netfs_attempt_statfs
	(
	struct iouser * cred,
	struct node * node,
	fsys_statfsbuf_t * st
	)
	{
	LOG_MSG("netfs_attempt_statfs");

	/*Operation is not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_statfs*/
/*----------------------------------------------------------------------------*/
/*Syncs the filesystem*/
error_t
netfs_attempt_syncfs
	(
	struct iouser * cred,
	int wait
	)
	{
	LOG_MSG("netfs_attempt_syncfs");

	/*Everythin OK*/
	return 0;
	}/*netfs_attempt_syncfs*/
/*----------------------------------------------------------------------------*/
/*Creates a link in `dir` with `name` to `file`*/
error_t
netfs_attempt_link
	(
	struct iouser * user,
	struct node * dir,
	struct node * file,
	char * name,
	int excl
	)
	{
	LOG_MSG("netfs_attempt_link");

	/*Operation not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_link*/
/*----------------------------------------------------------------------------*/
/*Attempts to create an anonymous file related to `dir` with `mode`*/
error_t
netfs_attempt_mkfile
	(
	struct iouser * user,
	struct node * dir,
	mode_t mode,
	struct node ** node
	)
	{
	LOG_MSG("netfs_attempt_mkfile");

	/*Unlock the directory*/
	mutex_unlock(&dir->lock);

	/*Operation not supported*/
	return EOPNOTSUPP;
	}/*netfs_attempt_mkfile*/
/*----------------------------------------------------------------------------*/
/*Reads the contents of symlink `node` into `buf`*/
error_t
netfs_attempt_readlink
	(
	struct iouser * user,
	struct node * node,
	char * buf
	)
	{
	LOG_MSG("netfs_attempt_readlink");

	/*Operation not supported (why?..)*/
	return EOPNOTSUPP;
	}/*netfs_attempt_readlink*/
/*----------------------------------------------------------------------------*/
/*Reads from file `node` up to `len` bytes from `offset` into `data`*/
error_t
netfs_attempt_read
	(
	struct iouser * cred,
	struct node * np,
	loff_t offset,
	size_t * len,
	void * data
	)
	{
	LOG_MSG("netfs_attempt_read");

	error_t err = 0;

	/*Obtain a pointer to the first byte of the supplied buffer*/
	char * buf = data;
	
	/*Try to read the requested information from the file*/
	err = io_read(np->nn->port, &buf, len, offset, *len);
	
	/*If some data has been read successfully*/
	if(!err && (buf != data))
		{
		/*copy the data from the buffer into which it has just been read into
			the supplied receiver*/
		memcpy(data, buf, *len);
		
		/*unmap the new buffer*/
		munmap(buf, *len);
		}

	/*Return the result of reading*/
	return err;
	}/*netfs_attempt_read*/
/*----------------------------------------------------------------------------*/
/*Writes to file `node` up to `len` bytes from offset from `data`*/
error_t
netfs_attempt_write
	(
	struct iouser * cred,
	struct node * node,
	loff_t offset,
	size_t * len,
	void * data
	)
	{
	LOG_MSG("netfs_attempt_write");

	return 0;
	}/*netfs_attempt_write*/
/*----------------------------------------------------------------------------*/
/*Frees all storage associated with the node*/
void
netfs_node_norefs
	(
	struct node * np
	)
	{
	/*Destroy the node*/
	node_destroy(np);
	}/*netfs_node_norefs*/
/*----------------------------------------------------------------------------*/
/*Implements file_get_translator_cntl as described in <hurd/fs.defs>
	(according to diskfs_S_file_get_translator_cntl)*/
kern_return_t
netfs_S_file_get_translator_cntl
	(
	struct protid * user,
	mach_port_t * cntl,
	mach_msg_type_name_t * cntltype
	)
	{
	/*If the information about the user is missing*/
	if(!user)
		return EOPNOTSUPP;
		
	error_t err = 0;
	
	/*Obtain the node for which we are called*/
	node_t * np = user->po->np;
	
	/*Lock the node*/
	mutex_lock(&np->lock);
	
	/*Check if the user is the owner of this node*/
	err = fshelp_isowner(&np->nn_stat, user->user);
	
	/*If no errors have happened*/
	if(!err)
		/*try to fetch the control port*/
		err = fshelp_fetch_control(&np->transbox, cntl);
		
	/*If no errors have occurred, but no port has been returned*/
	if(!err && (cntl == MACH_PORT_NULL))
		/*set the error accordingly*/
		err = ENXIO;
		
	/*If no errors have occurred so far*/
	if(!err)
		/*set the control port type*/
		*cntltype = MACH_MSG_TYPE_MOVE_SEND;
		
	/*Unlock the node*/
	mutex_unlock(&np->lock);
	
	/*Return the result of operations*/
	return err;
	}/*netfs_S_file_get_translator_cntl*/
/*----------------------------------------------------------------------------*/
/*Entry point*/
int
main
	(
	int argc,
	char ** argv
	)
	{
	/*Start logging*/
	INIT_LOG();
	LOG_MSG(">> Starting initialization...");

	/*The port on which this translator will be set upon*/
	mach_port_t bootstrap_port;
	
	error_t err = 0;
	
	/*Parse the command line arguments*/
	argp_parse(&argp_startup, argc, argv, ARGP_IN_ORDER, 0, 0);
	if(target_name == NULL)
		{
		char * p = *argv + strlen(*argv);
		
		/*find where the name of this executable starts (it should match \-.*)*/
		for(; p >= *argv; --p)
			if(*p == '/')
				{
				if((p > *argv) && (p[-1] == '\\'))
					continue; /*this is an escaped '/'*/
				/*we've reached the beginning of the name*/
				break;
				}

		if(*p == '/')
			++p;
		
		if(*p == '-') /*actually, any filter translator must have a name starting
									with '-'*/
			++p;
			
		target_name = p;
		}
	LOG_MSG("Command line arguments parsed. Target name: '%s'.", target_name);
	
	/*Try to create the root node*/
	err = node_create_root(&netfs_root_node);
	if(err)
		error(EXIT_FAILURE, err, "Failed to create the root node");
	LOG_MSG("Root node created.");

	/*Obtain the bootstrap port*/
	task_get_bootstrap_port(mach_task_self(), &bootstrap_port);

	/*Initialize the translator*/
	netfs_init();

	/*Obtain a port to the underlying node opened as O_NOTRANS*/
	underlying_node = netfs_startup(bootstrap_port, O_READ | O_NOTRANS);
	LOG_MSG("netfs initialization complete.");

	/*Initialize the root node*/
	err = node_init_root(underlying_node, netfs_root_node);
	if(err)
		error(EXIT_FAILURE, err, "Failed to initialize the root node");
	LOG_MSG("Root node initialized.");
	LOG_MSG("\tRoot node address: 0x%lX", (unsigned long)netfs_root_node);
	
	/*Map the time for updating node information*/
	err = maptime_map(0, 0, &maptime);
	if(err)
		error(EXIT_FAILURE, err, "Failed to map the time");
	LOG_MSG("Time mapped.");
		
	/*Obtain stat information about the underlying node*/
	err = io_stat(underlying_node, &underlying_node_stat);
	if(err)
		error(EXIT_FAILURE, err,
			"Cannot obtain stat information about the underlying node");
	LOG_MSG("Stat information for undelying node obtained.");
	
	/*Obtain the ID of the current process*/
	fsid = getpid();
	
	/*Setup the stat information for the root node*/
	netfs_root_node->nn_stat = underlying_node_stat;

	netfs_root_node->nn_stat.st_ino 	= FILTER_ROOT_INODE;
	netfs_root_node->nn_stat.st_fsid	= fsid;
	netfs_root_node->nn_stat.st_mode = underlying_node_stat.st_mode;
		
	netfs_root_node->nn_translated = netfs_root_node->nn_stat.st_mode;
	
	/*Filter the translator stack under ourselves*/
	err = trace_find(underlying_node, "/hurd/m", O_READ, &target);
	if(err)
		error
			(
			EXIT_FAILURE, err,
			"Could not trace the translator stack on the underlying node"
			);
			
	netfs_root_node->nn->port = target;
	
	/*Update the timestamps of the root node*/
	fshelp_touch
		(&netfs_root_node->nn_stat, TOUCH_ATIME | TOUCH_MTIME | TOUCH_CTIME,
		maptime);

	LOG_MSG(">> Initialization complete. Entering netfs server loop...");
	
	/*Start serving clients*/
	for(;;)
		netfs_server_loop();
	}/*main*/
/*----------------------------------------------------------------------------*/
