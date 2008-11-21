/*---------------------------------------------------------------------------*/
/*options.h*/
/*---------------------------------------------------------------------------*/
/*Definitions for parsing the command line switches*/
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

/*---------------------------------------------------------------------------*/
#define _GNU_SOURCE 1
/*---------------------------------------------------------------------------*/
#include <argp.h>
#include <error.h>
/*---------------------------------------------------------------------------*/
#include "debug.h"
#include "options.h"
#include "node.h"
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*--------Macros-------------------------------------------------------------*/
/*Short documentation for argp*/
#define ARGS_DOC	"TARGET-NAME"
#define DOC 			"Finds the bottommost translator called TARGET-NAME in the \
static stack of translators and reads and write to it."
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*--------Forward Declarations-----------------------------------------------*/
/*Argp parser function for the common options*/
static
  error_t
  argp_parse_common_options (int key, char *arg, struct argp_state *state);
/*---------------------------------------------------------------------------*/
/*Argp parser function for the startup options*/
static
  error_t
  argp_parse_startup_options (int key, char *arg, struct argp_state *state);
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*--------Global Variables---------------------------------------------------*/
/*This variable is set to a non-zero value after the parsing of starup
  options is finished*/
static int parsing_startup_options_finished;
/*---------------------------------------------------------------------------*/
/*Argp options common to both the runtime and the startup parser*/
static const struct argp_option argp_common_options[] = {
  {0}
};

/*---------------------------------------------------------------------------*/
/*Argp options only meaningful for startupp parsing*/
static const struct argp_option argp_startup_options[] = {
  {0}
};

/*---------------------------------------------------------------------------*/
/*Argp parser for only the common options*/
static const struct argp argp_parser_common_options =
  { argp_common_options, argp_parse_common_options, 0, 0, 0 };
/*---------------------------------------------------------------------------*/
/*Argp parser for only the startup options*/
static const struct argp argp_parser_startup_options =
  { argp_startup_options, argp_parse_startup_options, 0, 0, 0 };
/*---------------------------------------------------------------------------*/
/*The list of children parsers for runtime arguments*/
static const struct argp_child argp_children_runtime[] = {
  {&argp_parser_common_options},
  {&netfs_std_runtime_argp},
  {0}
};

/*---------------------------------------------------------------------------*/
/*The list of children parsers for startup arguments*/
static const struct argp_child argp_children_startup[] = {
  {&argp_parser_startup_options},
  {&argp_parser_common_options},
  {&netfs_std_startup_argp},
  {0}
};

/*---------------------------------------------------------------------------*/
/*The version of the server for argp*/
const char *argp_program_version = "0.0";
/*---------------------------------------------------------------------------*/
/*The arpg parser for runtime arguments*/
struct argp argp_runtime = { 0, 0, 0, 0, argp_children_runtime };

/*---------------------------------------------------------------------------*/
/*The argp parser for startup arguments*/
struct argp argp_startup = { 0, 0, ARGS_DOC, DOC, argp_children_startup };

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*--------Functions----------------------------------------------------------*/
/*Argp parser function for the common options*/
static
  error_t
  argp_parse_common_options (int key, char *arg, struct argp_state *state)
{
  error_t err = 0;

  /*Go through the possible options */
  switch (key)
    {
    case ARGP_KEY_END:
      {
	/*If parsing of startup options has not finished */
	if (!parsing_startup_options_finished)
	  {
	    /*set the flag that the startup options have already been parsed */
	    parsing_startup_options_finished = 1;
	  }
	else
	  {
	  }

	break;
      }
    case ARGP_KEY_ARG:		// the translator to filter out;
      {
	target_name = strdup (arg);
	if (!target_name)
	  error (EXIT_FAILURE, ENOMEM, "argp_parse_common_options: "
		 "Could not strdup the translator name");

	break;
      }
      /*If the option could not be recognized */
    default:
      {
	/*set the error code */
	err = ARGP_ERR_UNKNOWN;
      }
    }

  /*Return the result */
  return err;
}				/*argp_parse_common_options */

/*---------------------------------------------------------------------------*/
/*Argp parser function for the startup options*/
static
  error_t
  argp_parse_startup_options (int key, char *arg, struct argp_state *state)
{
  /*Do nothing in a beautiful way */
  error_t err = 0;

  switch (key)
    {
    default:
      {
	err = ARGP_ERR_UNKNOWN;

	break;
      }
    }

  return err;
}				/*argp_parse_startup_options */

/*---------------------------------------------------------------------------*/
