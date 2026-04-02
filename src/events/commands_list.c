#include "events/commands.h"
#include "events/events.h"
#include "events/chat.h"

#include "directives/dmath.h"
#include "directives/dword.h"
#include "directives/dfree.h"

#include "player/movement.h"
#include "player/camera.h"

#include "text/text_mgr.h"
#include "text/text_obj.h"

#include "graphics/glfw.h"

#include "values/state.h"

#include "world/modify.h"
#include "world/map.h"
#include "world/sky.h"

#include "io/format.h"
#include "io/logs.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VX_UNUSED(x) do { (void)(x); } while (0)

/* Define a command action - what to do when executed normally. */
#define ACT_FUNC(name) static int vxcl_##name##_act(int argc, char **argv)
/* Define a command query - values to print related to the main action
   For example, querying the /time command will display the current time. */
#define QRY_FUNC(name) static char *vxcl_##name##_qry(VX_NO_ARG)

/* Get the given argument index as an integral type. */
#define ASINUM(type, index) (VX_CAST(type, strtoimax(argv[index], VX_NULL, 0)))
/* Get the given argument index as a floating-point type. */
#define ASFNUM(type, index) (VX_CAST(type, strtod(argv[index], VX_NULL)))

static int vxcmd_validate_given_impl(char *value)
{
	if (strchr(value, '~')) return 0;
	const double conv_res = strtod(value, VX_NULL);
	if (conv_res == 0.0 && !(strlen(value) == VX_CAST(size_t, vxfmt_count_char(value, '0') + vxfmt_contains_char(value, '.')))) return 0;
	return 1;
}

/* Command argument numerical representation check. */
#define VALIDATENUM(index) do { if (!vxcmd_validate_given_impl(argv[index])) return index; } while (0)

static int vxcmd_expand_tilde(char **target_argv_addr, char *value_as_str);

static int vxcmd_warn_index_oob(int index, int argc)
{
	if (index < argc) return 0;
	vxlog_free(VX_LOG_WARNING_BIT, vxfmt_text("Argument index %d is out of range, %d argument(s) given", index, argc));
	return 1;
}

static int vxcmd_convert_floating_impl(char **argv, int argc, int index, double true_value)
{
	if (vxcmd_warn_index_oob(index, argc)) return 1;
	return vxcmd_expand_tilde(argv + index, vxfmt_text("%f", VX_CAST(double, true_value)));
}
static int vxcmd_convert_integral_impl(char **argv, int argc, int index, intmax_t true_value)
{
	if (vxcmd_warn_index_oob(index, argc)) return 0;
	return vxcmd_expand_tilde(argv + index, vxfmt_text("%" PRIiMAX, VX_CAST(intmax_t, true_value)));
}

/* Use the tilde rules to change the value of the given argument index appropriately (for floating-point types). */
#define CONVFLT(index, true_value) do { if (!vxcmd_convert_floating_impl(argv, argc, index, VX_CAST(double, true_value))) return index; } while (0)
/* Use the tilde rules to change the value of the given argument index appropriately (for integral types). */
#define CONVINT(index, true_value) do { if (!vxcmd_convert_integral_impl(argv, argc, index, VX_CAST(intmax_t, true_value))) return index; } while (0)


ACT_FUNC(tp)
{
	CONVFLT(0, vxplr_inst.pos.x);
	CONVFLT(1, vxplr_inst.pos.y);
	CONVFLT(2, vxplr_inst.pos.z);

	const dvec3 pos = { ASFNUM(double, 0), ASFNUM(double, 1), ASFNUM(double, 2) };
	vxplr_move_set_position(&pos);

	if (argc < 4) return VX_CMD_SUCCESS;

	CONVFLT(3, vxplr_cam.yaw);
	vxplr_cam.yaw = ASFNUM(double, 3);
	if (argc > 4) {
		CONVFLT(4, vxplr_cam.pitch);
		vxplr_cam.pitch = ASFNUM(double, 4);
	}
	vxplr_cam_dirs_update();

	return VX_CMD_SUCCESS;
}
QRY_FUNC(tp)
{
	return vxfmt_text(
		"Position: %.3f, %.3f, %.3f\nDirection: Y: %.3f, P: %.3f",
		vxplr_inst.pos.x,
		vxplr_inst.pos.y,
		vxplr_inst.pos.z,
		vxplr_cam.yaw,
		vxplr_cam.pitch
	);
}

ACT_FUNC(fill)
{
	VX_UNUSED(argc);
	CONVINT(0, vxplr_inst.pos.x);
	CONVINT(1, vxplr_inst.pos.y);
	CONVINT(2, vxplr_inst.pos.z);

	CONVINT(3, vxplr_inst.pos.x);
	CONVINT(4, vxplr_inst.pos.y);
	CONVINT(5, vxplr_inst.pos.z);

	VALIDATENUM(6);
	const vxblk target_block = ASINUM(vxblk, 6);

	wpos from = { ASINUM(intmax_t, 0), ASINUM(intmax_t, 1), ASINUM(intmax_t, 2) };
	const wpos to = { ASINUM(intmax_t, 3), ASINUM(intmax_t, 4), ASINUM(intmax_t, 5) };
	const wpos step = { to.x > from.x ? 1 : -1, to.z > from.z ? 1 : -1, to.z > from.z ? 1 : -1 };

	for (; from.x != to.x; from.x += step.x) {
		for (; from.y != to.y; from.y += step.y) {
			for (; from.z != to.z; from.z += step.z) {
				vxwld_set(&from, target_block);
			}
		}	
	}

	return VX_CMD_SUCCESS;
}

ACT_FUNC(set)
{
	VX_UNUSED(argc);

	CONVINT(0, vxplr_inst.pos.x);
	CONVINT(1, vxplr_inst.pos.y);
	CONVINT(2, vxplr_inst.pos.z);

	VALIDATENUM(3);

	const wpos target = { ASINUM(intmax_t, 0), ASINUM(intmax_t, 1), ASINUM(intmax_t, 2) };
	vxwld_set(&target, ASINUM(vxblk, 1));

	return VX_CMD_SUCCESS;
}

ACT_FUNC(refrate)
{
	VX_UNUSED(argc);
	glfwSwapInterval(ASINUM(int, 0));
	return VX_CMD_SUCCESS;
}

ACT_FUNC(time)
{
	VX_UNUSED(argc);

	CONVFLT(0, VX_CAST(double, vxstate_vals.world_day_counter) * VX_SKY_DAY_SECONDS + vxstate_vals.cycle_day_seconds);
	const double given_seconds = ASFNUM(double, 0);

	vxstate_vals.world_day_counter = VX_CAST(intmax_t, given_seconds / VX_SKY_DAY_SECONDS);
	vxstate_vals.cycle_day_seconds = given_seconds - (VX_CAST(double, vxstate_vals.world_day_counter) * VX_SKY_DAY_SECONDS);

	return VX_CMD_SUCCESS;
}
QRY_FUNC(time)
{
	return vxfmt_text(
		"Day: %" PRIiMAX "\nLocal time: %d\nGlobal time: %d",
		vxstate_vals.world_day_counter, vxstate_vals.cycle_day_seconds,
		VX_CAST(double, vxstate_vals.world_day_counter) * VX_SKY_DAY_SECONDS + vxstate_vals.cycle_day_seconds
	);
}

ACT_FUNC(speed)
{
	VX_UNUSED(argc);
	CONVFLT(0, vxplr_inst.base_speed);
	vxplr_inst.base_speed = ASFNUM(double, 0);
	return VX_CMD_SUCCESS;
}
QRY_FUNC(speed)
{
	return vxfmt_text(
		"Base movement speed: %.3f\n",
		vxplr_inst.base_speed
	);
}

ACT_FUNC(tick)
{
	VX_UNUSED(argc);
	CONVFLT(0, vxstate_vals.game_tick_speed);
	vxstate_vals.game_tick_speed = VX_CLAMP(ASFNUM(double, 0), VX_GAME_TICK_MIN, VX_GAME_TICK_MAX);
	return VX_CMD_SUCCESS;
}
QRY_FUNC(tick)
{
	return vxfmt_text("Current game tick speed: %.3f", vxstate_vals.game_tick_speed);
}

ACT_FUNC(sens)
{
	VX_UNUSED(argc);
	CONVFLT(0, vxplr_cam.sensitivity);
	vxplr_cam.sensitivity = ASFNUM(double, 0);
	return VX_CMD_SUCCESS;
}
QRY_FUNC(sens)
{
	return vxfmt_text("Relative mouse sensitivity: %.3f", vxplr_cam.sensitivity);
}

ACT_FUNC(rd)
{
	VX_UNUSED(argc);
	CONVINT(0, vxwld_info.render_distance);
	VALIDATENUM(0);
	vxwld_change_rdist(ASINUM(pos_type, 0));
	return VX_CMD_SUCCESS;
}
QRY_FUNC(rd)
{
	return vxfmt_text("Render distance: %" PRIiMAX, vxwld_info.render_distance);
}

ACT_FUNC(fov)
{
	VX_UNUSED(argc);
	CONVFLT(0, vxplr_cam.fov);
	vxplr_cam_update_fov(ASFNUM(double, 0));
	return VX_CMD_SUCCESS;
}
QRY_FUNC(fov)
{
	return vxfmt_text(
		"FOV degrees: %.3f\nFOV radians: %.3f",
		vxplr_cam.fov * VX_DEGREE_MULT,
		vxplr_cam.fov
	);
}

ACT_FUNC(clear)
{
	VX_UNUSED(argv);
	VX_UNUSED(argc);
	vxtxt_obj_clear_text(&vxchat_text);
	return VX_CMD_SUCCESS;
}

ACT_FUNC(text)
{
	VX_UNUSED(argv);
	VX_UNUSED(argc);
	char printable_ascii[('~' - '!') + 1];
	for (int i = 0; i < VX_CAST(int, sizeof printable_ascii); ++i) printable_ascii[i] = VX_CAST(char, '!' + i);
	vxchat_add_text(printable_ascii);
	return VX_CMD_SUCCESS;
}

ACT_FUNC(close)
{
	VX_UNUSED(argv);
	VX_UNUSED(argc);
	vxtg_toggles.loop_active = 0;
	return VX_CMD_SUCCESS;
}


ACT_FUNC(help)
{
	VX_UNUSED(argv);
	VX_UNUSED(argc);

	VALIDATENUM(0);

	/* Command syntax/format information text. */
	const char *base_info_txt =
		"Commands start with a '/' and are formatted as such: /name arg1 arg2... "
		"Optional arguments are marked with a '*': '/name arg1 *arg2'. "
		"Some commands display related values if run without arguments: '/time' displays the in-game time. "
		"A tilde (~) refers to the current value (if available), and can be used with operations: "
		"'/tick ~-5' -> Decreases tick by 5; '/tick -~10' adds 10 then negates.";
	/* Page 1 = base information, page 2 onwards is command descriptions. */
	int page = ASINUM(int, 0);
	page = VX_INT_MAX(page, 1) - 1;

	char *combined_cmd_text = VX_NULL;
	size_t cmds_index = 0u, cmds_ptr_count = vxcmd_objs_count;

	/* Skip loop, copy base text to chat. */
	if (page == 1) cmds_ptr_count = 0u;
	else {
		cmds_index = VX_CAST(size_t, page - 2) * VX_CAST(size_t, vxtxt_manager.chat_lines_limit);
		const size_t target_cmd_end = cmds_index + VX_CAST(size_t, vxtxt_manager.chat_lines_limit);
		cmds_ptr_count = VX_INT_MIN(cmds_ptr_count, target_cmd_end);
	}

	/* Add descriptions of specific commands for this page. */
	for (; cmds_index < cmds_ptr_count; ++cmds_index) {
		const struct vxcmd_instance_obj *cur_cmd = vxcmd_objs + cmds_index;
		if (*cur_cmd->description == '_') continue;

		char *concat_res = vxfmt_concat_allocd_free(
			VX_FMT_CONCAT_RESULTS_FIRST,
			&combined_cmd_text,
			vxfmt_text("/%s %s -- Arguments: %s%c",
				cur_cmd->base_name,
				cur_cmd->description,
				vxcmd_get_args_text(cur_cmd),
				"\n"[cmds_index == (cmds_ptr_count - 1)]
			)
		);

		if (concat_res) continue;
		VX_FREE(combined_cmd_text);
		return VX_CMD_ERROR;
	}

	char *formatted_result;
	if (combined_cmd_text) formatted_result = vxchat_fmt_chat(combined_cmd_text);
	else formatted_result = vxchat_fmt_chat(vxfmt_copy_str(base_info_txt));
	
	/* Help text is very large, so just set the text instead of adding. */
	vxtxt_obj_set_text(&vxchat_text, formatted_result, 0);

	return VX_CMD_SUCCESS;
}


static int vxcmd_expand_tilde(char **target_argv_addr, char *value_as_str)
{
	if (!target_argv_addr || !value_as_str) return 0;

	const int is_floating_num = vxfmt_contains_char(value_as_str, '.');

	const char *tilde_ptr_index = strchr(*target_argv_addr, '~');
	int expression_sign;

	/* No tilde, only check for validity. */
	if (!tilde_ptr_index) return vxfmt_is_numeric(*target_argv_addr) ? 1 : 0;

	if (tilde_ptr_index == *target_argv_addr) expression_sign = 1; /* No sign change if tilde is at the very start. */
	else if (tilde_ptr_index - *target_argv_addr != 1) return 0; /* Tilde can only be at the start or after starting 'sign character'. */
	else {
		char supposed_sign_char = tilde_ptr_index[-1];
		if (supposed_sign_char == '+') expression_sign = 1;
		else if (supposed_sign_char == '-') expression_sign = -1;
		else return 0; /* The alleged 'sign character' before the tilde isn't actually one. */
	}

	/* Check if there is anything after the tilde - will need to be a valid numeric value if so. */
	if (tilde_ptr_index && *(++tilde_ptr_index) && !vxfmt_is_numeric(tilde_ptr_index)) return 0;

	/* Combine found values. */
	if (is_floating_num) {
		const double after_tilde_number = strtod(tilde_ptr_index, VX_NULL);
		const double tilde_value = strtod(value_as_str, VX_NULL);
		*target_argv_addr = vxfmt_text("%f", (after_tilde_number + tilde_value) * VX_CAST(double, expression_sign));
	} else {
		const intmax_t after_tilde_number = strtoimax(tilde_ptr_index, VX_NULL, 0);
		const intmax_t tilde_value = strtoimax(value_as_str, VX_NULL, 0);
		*target_argv_addr = vxfmt_text("%" PRIiMAX, (after_tilde_number + tilde_value) * expression_sign);
	}

	return VX_CMD_SUCCESS;
}


/* For the arguments list text, any optional arguments must follow required arguments.
   Debug commands are not shown in the '/help' command. They are marked with an initial underscore ('_') in the description. */

const struct vxcmd_instance_obj vxcmd_objs[] = {
{
	"tp",
	"Teleports you to the specified x, y, z position. Optionally also sets the camera's direction.",
	"x y z *yaw *pitch",
	vxcl_tp_act, vxcl_tp_qry
},
{
	"fill",
	"Fills between both coordinates inclusive with the specified block ID.",
	"x1 y1 z1 x2 y2 z2 block",
	vxcl_fill_act, VX_NULL
},
{
	"set",
	"Changes the block at the given position.",
	"x y z block",
	vxcl_set_act, VX_NULL,
},
{
	"refrate",
	"Sets the number of screen updates to wait before swapping buffers.",
	"intervals",
	vxcl_refrate_act, VX_NULL,
},
{
	"time",
	"Changes the in-game time in seconds.",
	"*secs",
	vxcl_time_act, vxcl_time_qry,
},
{
	"speed",
	"Changes your movement speed.",
	"*speed",
	vxcl_speed_act, vxcl_speed_qry,
},
{
	"tick",
	"Changes the natural tick speed.",
	"*tick",
	vxcl_tick_act, vxcl_tick_qry,
},
{
	"sens",
	"Changes the in-game mouse sensitivity.",
	"*sens",
	vxcl_sens_act, vxcl_sens_qry,
},
{
	"rd",
	"Changes the world render distance.",
	"*rd",
	vxcl_rd_act, vxcl_rd_qry,
},
{
	"fov",
	"Sets the camera FOV in degrees.",
	"*fov",
	vxcl_fov_act, vxcl_fov_qry,
},
{
	"clear",
	"Clears the chat.",
	VX_NULL,
	vxcl_clear_act, VX_NULL,
},
{
	"text",
	"_Text renderer testing.",
	VX_NULL,
	vxcl_text_act, VX_NULL,
},
{
	"close",
	"Closes the game.",
	VX_NULL,
	vxcl_close_act, VX_NULL,
},
{
	"help",
	"Prints out some helpful text. Page 1 for command format, others for commands descriptions.",
	"*page",
	vxcl_help_act, VX_NULL,
}
};

const unsigned short vxcmd_objs_count = sizeof vxcmd_objs / sizeof *vxcmd_objs;
