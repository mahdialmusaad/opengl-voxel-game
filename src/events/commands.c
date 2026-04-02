#include "events/commands.h"
#include "events/chat.h"

#include "directives/dcast.h"
#include "directives/dfree.h"

#include "player/inventory.h"

#include "text/text_obj.h"

#include "values/state.h"

#include "io/format.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

vxtxt_obj vxcmd_text;

void vxcmd_add_text(const char *append)
{
	if (!append) return;
	vxtxt_obj_set_text(&vxcmd_text, vxfmt_concat_allocd(VX_FMT_CONCAT_DEFAULT, &vxcmd_text.text, append), 0);
}

void vxcmd_begin(int is_normal_message)
{
	vxplr_inv_toggle(0);
	vxtg_toggles.chatting = 1;
	vxtg_toggles.show_any_gui = 1;
	vxtg_toggles.ignore_first = is_normal_message == 1;
	vxtg_toggles.not_command_chat = is_normal_message == 1;

	vxtxt_obj_set_transparency(&vxcmd_text, 1);
	if (!VX_TEXT_EMPTY(vxchat_text)) vxtxt_obj_set_transparency(&vxchat_text, 1.0f);
}
void vxcmd_hide_texts(int fade_out)
{
	vxtxt_obj_clear_text(&vxcmd_text);
	if (fade_out) vxtxt_obj_fade_timer(&vxchat_text, VX_TEXT_DEFAULT_FADE_TIME);
	else if (!VX_TEXT_EMPTY(vxchat_text)) vxtxt_obj_set_transparency(&vxchat_text, 0.0f);
	vxtg_toggles.chatting = 0;
}

const char *vxcmd_get_args_text(const struct vxcmd_instance_obj *cmd)
{
	return cmd->arguments_names_list ? cmd->arguments_names_list : "";
}


/* Whether the given number of arguments is valid for the given command. */
static int vxcmd_args_match(const struct vxcmd_instance_obj *cmd, int given_args_count)
{
	if (!cmd->arguments_names_list || !strlen(cmd->arguments_names_list)) return given_args_count == (cmd->query_function ? 1 : -1);
	const int total_args = vxfmt_count_char(cmd->arguments_names_list, ' ') + 1;
	const int optionals_count = vxfmt_count_char(cmd->arguments_names_list, '*');

	if (optionals_count) return given_args_count >= (total_args - optionals_count);
	else return given_args_count == total_args;
}

void vxcmd_execute(VX_NO_ARG)
{
	char *command_text = vxcmd_text.text;
	if (!*command_text) return;

	char *chat_copy = vxfmt_copy_str(command_text);
	if (!chat_copy) return;

	if (*command_text != '/') {
		vxchat_add_text_free(vxfmt_text("[YOU] %s", command_text));
		return;
	}

	const char *cmd_name = command_text + 1;
	const size_t cmd_length = VX_CAST(size_t, vxfmt_const_strchrnul(command_text, ' ') - cmd_name);

	const struct vxcmd_instance_obj *found_cmd_ptr = vxcmd_objs;
	const struct vxcmd_instance_obj *end_cmd_ptr = vxcmd_objs + vxcmd_objs_count;
	while (found_cmd_ptr != end_cmd_ptr) {
		const size_t other_length = strlen(found_cmd_ptr->base_name);
		if (other_length == cmd_length && strncmp(found_cmd_ptr->base_name, cmd_name, cmd_length) == 0) break;
		++found_cmd_ptr;
	}

	if (found_cmd_ptr == end_cmd_ptr) {
		vxchat_add_text_free(vxfmt_text("'%.*s' is not a valid command. Type /help for help.", cmd_length + 1u, command_text));
		return;
	}

	int command_error_code = VX_CMD_ERROR;
	
	int argc = vxfmt_count_char(command_text, ' ');
	char **argv = argc ? VX_CAST(char **, malloc(sizeof *argv * VX_CAST(size_t, argc))) : VX_NULL, *args_cpy = VX_NULL;
	if (!argv && argc) goto command_immediate_fail;
	
	args_cpy = vxfmt_copy_str(cmd_name + cmd_length);
	if (!argv) goto command_immediate_fail;

	/* Add pointers of text to argv list and ignore any arguments with only spaces. */
	for (int i = 0; i < argc; ++i) {
		char *next_space = vxfmt_strchrnul(i ? argv[i - 1] : args_cpy, ' ');
		if (!*next_space) goto command_immediate_fail;
		*next_space++ = '\0'; /* Terminator between arguments so they seem like separate strings. */
		if (*next_space == ' ' || !*next_space) --argc; /* Still another space or reached the end of the string; not a valid argument. */
		else argv[i] = next_space;
	}

	/* Has a query function AND player gave no arguments, so run associated query function. */
	if (found_cmd_ptr->query_function && argc == 0) {
		vxchat_add_text_free(found_cmd_ptr->query_function());
		return;
	}

	/* Check for incorrect arguments count or '?' argument, display help if so. */
	if ((argc == 1 && (strcmp(*argv, "?") == 0)) || (!vxcmd_args_match(found_cmd_ptr, argc))) {
		const char *args_text = vxcmd_get_args_text(found_cmd_ptr);
		vxchat_add_text_free(vxfmt_text("Usage: /%s%s%s", found_cmd_ptr->base_name, *args_text ? " " : args_text, args_text));
		return;
	}

	command_error_code = found_cmd_ptr->action_function(argc, argv);

	switch (command_error_code) {
		case VX_CMD_SUCCESS:
			break;
		case VX_CMD_ERROR:
		command_immediate_fail:
			vxchat_add_text("Generic failure while executing command. Please try again.");
			break;
		default:
			vxchat_add_text_free(vxfmt_text("Invalid '%s' at index %d.", argv[command_error_code], command_error_code + 1));
			break;
	}

	VX_FREE(argv);
}
