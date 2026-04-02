#pragma once
#ifndef SOURCE_EVENTS_COMMAND_VXL_HDR
#define SOURCE_EVENTS_COMMAND_VXL_HDR

#include "directives/dextern.h"

#include "text/text_obj.h"

/* List of all chat commands. */
extern struct vxcmd_instance_obj
{
	/* Command name without the initial forward-slash. */
	const char *base_name;
	/* Helpful description on the purpose of this command.
	   Prefix with '_' to exclude from help list. */
	const char *description;
	/* Argument list.
	   Format can be seen in help text or in markdown. */
	const char *arguments_names_list;
	/* Main function of the command.
	   Returns:
	   * VX_CMD_SUCCESS on success or completion;
	   * VX_CMD_ERROR on a generic failure;
	   * or the argument index which caused the error. */
	int (*action_function)(int argc, char **argv);
	/* Function for retrieving used values. */
	char *(*query_function)(VX_NO_ARG);
} const vxcmd_objs[];
/* Number of chat commands. */
extern const unsigned short vxcmd_objs_count;

/* Inputted command text object. */
extern vxtxt_obj vxcmd_text;

/* Command successfully executed. */
#define VX_CMD_SUCCESS (-1)
/* Command generic failure (e.g. allocation). */
#define VX_CMD_ERROR (-2)

VX_C_START

/* Append text to the command input. */
void vxcmd_add_text(const char *append);

/* Open command GUI in either message or command mode and start getting input. */
void vxcmd_begin(int is_normal_message);

/* Hide the command interface either immediately or through fading. */
void vxcmd_hide_texts(int fade_out);

/* Return friendly text for the given command's arguments. */
const char *vxcmd_get_args_text(const struct vxcmd_instance_obj *cmd);

/* Initialize command-related values. Required only once. */
void vxcmd_init(VX_NO_ARG);
/* Execute the command stored in the text. */
void vxcmd_execute(VX_NO_ARG);

VX_C_END

#endif
