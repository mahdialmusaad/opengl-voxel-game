#define VX_GAME_TITLE "Voxels 1.0.6"
#define VX_COPYRIGHT_TITLE "Copyright (C) 2026 Mahdi Almusaad"
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "directives/dcast.h"
#include "directives/dmath.h"
#include "directives/dfree.h"

#include "graphics/glfuncs.h"
#include "graphics/glenum.h"
#include "graphics/glctx.h"
#include "graphics/glfw.h"

#include "player/inventory.h"
#include "player/movement.h"
#include "player/raycast.h"
#include "player/camera.h"

#include "events/commands.h"
#include "events/window.h"
#include "events/events.h"
#include "events/keys.h"
#include "events/chat.h"

#include "values/elements.h"
#include "values/state.h"

#include "shaders/loader.h"
#include "shaders/ubo.h"

#include "text/text_mgr.h"
#include "text/text_obj.h"

#include "values/state.h"

#include "world/locate.h"
#include "world/map.h"
#include "world/sky.h"

#include "utils/thread.h"
#include "utils/noise.h"

#include "vector/vec3.h"
#include "vector/mat4.h"

#include "io/format.h"
#include "io/files.h"
#include "io/logs.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define VX_FILE_ID "main.c"

/* Main text objects. */
static struct { vxtxt_obj static_info_txt, game_info_txt, world_info_txt; } vxmain_texts;

/* Main loop counters. */
static struct {
	int current, lowest, average, average_counter;
	double text_upd_waited, events_waited_time, largest_frame_time, prev_global_time;
} vxmain_counters;

static struct { int exit_immediate; } vxmain_args;

/* Update UBO values that change per frame, and clear depth + colour buffer. */
static void vxmain_update_ubos(VX_NO_ARG)
{
	static mat4 perspective_mat, origin_mat, view_mat;
	const vec3 stars_rotation_axis = { 1.0f, 1.0f, 1.0f }, planets_rotation_axis = { -1.0f, 0.0f, 0.0f };
	const float rotate_angle = VX_CAST(float, vxstate_vals.cycle_day_seconds / (VX_SKY_DAY_SECONDS / VX_TWO_PI));
	const vec3 sky_full_bright = { 0.45f, 0.72f, 0.98f }, sky_full_dark = { 0.0f, 0.0f, 0.05f }, sky_twilight_col = { 0.788f, 0.533f, 0.278f };

	const float relative_progress = VX_CAST(float, vxstate_vals.cycle_day_seconds / VX_SKY_DAY_SECONDS);
	const float relative_time = 2.0f * fabsf(relative_progress - floorf(relative_progress + 0.5f));
	const float chunk_size = (VX_WLD_CHUNK_XBLKS + VX_WLD_CHUNK_YBLKS + VX_WLD_CHUNK_ZBLKS) / 3.0f;
	const float stars_trnsp = (relative_time - VX_SKY_STARS_CYCLE_THRESHOLD) * (1.0f / (1.0f - VX_SKY_STARS_CYCLE_THRESHOLD));

	vxubo_list.floats.FLT_aspect = vxstate_vals.aspect;
	vxubo_list.floats.FLT_ctime = relative_time;
	vxubo_list.floats.FLT_stars_trnsp = stars_trnsp;
	vxubo_list.floats.FLT_gtime = VX_CAST(float, vxstate_vals.global_time);
	vxubo_list.floats.FLT_clouds_col = 1.1f - relative_time;
	vxubo_list.floats.FLT_fog_range = 2.0f / VX_CAST(float, vxwld_info.render_distance);
	vxubo_list.floats.FLT_fog_end =
		((vxubo_list.floats.FLT_fog_range / chunk_size) + chunk_size) +
		(relative_time * -chunk_size * 0.1f) + (vxtg_toggles.hide_fog * 1e10f);

	vxplr_cam_origin_matrix(&origin_mat);
	mat4_perspective(
		&perspective_mat,
		VX_CAST(float, vxplr_cam.fov * (vxkey_states[GLFW_KEY_G] ? vxplr_cam.zoom_mult : 1.0)),
		vxstate_vals.aspect, VX_CAST(float, VX_FRUSTUM_NEAR), VX_CAST(float, VX_FRUSTUM_FAR)
	);
	mat4_mul(&vxubo_list.mat4s.M4_origin, &perspective_mat, &origin_mat);
	mat4_mul(&vxubo_list.mat4s.M4_camera, &perspective_mat, vxplr_cam_matrix(&view_mat));

	/* Fixed FOV for axis matrix. */
	perspective_mat.vx.x = 1.0f / vxstate_vals.aspect;
	perspective_mat.vy.y = 1.0f;
	mat4_mul(&vxubo_list.mat4s.M4_axis, &perspective_mat, &origin_mat);

	if (stars_trnsp > 0.0f) mat4_rotate(&vxubo_list.mat4s.M4_stars, &vxubo_list.mat4s.M4_origin, rotate_angle, &stars_rotation_axis);
	mat4_rotate(&vxubo_list.mat4s.M4_planets, &vxubo_list.mat4s.M4_origin, rotate_angle, &planets_rotation_axis);

	wpos region_pos;
	vxwld_regoff_from_globoff(&vxplr_inst.offset, &region_pos);
	vxwld_regoff_globpos(&region_pos, &region_pos);

	#define VX_VECTOR_SET(name, vec, mulx, muly, mulz)\
	vxubo_list.vec4s.name.v.x = VX_CAST(float, vxplr_inst.pos.x - VX_CAST(double, vec.x * mulx));\
	vxubo_list.vec4s.name.v.y = VX_CAST(float, vxplr_inst.pos.y - VX_CAST(double, vec.y * muly));\
	vxubo_list.vec4s.name.v.z = VX_CAST(float, vxplr_inst.pos.z - VX_CAST(double, vec.z * mulz))

	const vec3 clouds_center_pos = { 0.0f, VX_SKY_CLOUDS_BASE_Y, vxubo_list.floats.FLT_gtime }; 
	VX_VECTOR_SET(V4_raycast_lpos, vxplr_ray.slctd_pos, 1, 1, 1);
	VX_VECTOR_SET(V4_chunk_lpos, vxplr_inst.offset, VX_WLD_CHUNK_XBLKS, VX_WLD_CHUNK_YBLKS, VX_WLD_CHUNK_ZBLKS);
	VX_VECTOR_SET(V4_region_lpos, region_pos, 1,1,1);
	VX_VECTOR_SET(V4_clouds_offset, clouds_center_pos, 1.0f, 1.0f, 1.0f);

	vec3 sky_colour, twilight_colour;
	vec3_lerp(&sky_colour, &sky_full_bright, &sky_full_dark, relative_time * relative_time * (3.0f - 2.0f * relative_time));
	memcpy(&vxubo_list.vec4s.V4_main_sky, &sky_colour, sizeof sky_colour);

	vxubo_list.vec4s.V4_world_light.v.x = vxubo_list.vec4s.V4_world_light.v.y = vxubo_list.vec4s.V4_world_light.v.z = 1.1f - relative_time;

	gl.ClearColor(sky_colour.x, sky_colour.y, sky_colour.z, 1.0f);
	gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	vxstate_vals.twilight_colour_trnsp = 1.0f + (-100.0f * (relative_time - 0.5f) * (relative_time - 0.5f));
	vec3_lerp(&twilight_colour, &sky_colour, &sky_twilight_col, vxstate_vals.twilight_colour_trnsp);
	memcpy(&vxubo_list.vec4s.V4_evening_sky, &twilight_colour, sizeof twilight_colour);

	VX_UBO_UPDATE(mat4s);
	VX_UBO_UPDATE(floats);
	VX_UBO_UPDATE(vec4s);
}

/* Update variables affected by the window aspect ratio or size. */
static void vxmain_aspect_changed(VX_NO_ARG)
{
	vxtxt_manager.text_width = VX_TEXT_DEFAULT_FONT_SIZE_WIDTH / vxstate_vals.aspect;
	vxtxt_mgr_all_dirty();

	if (!vxtg_toggles.in_world_instance) return;

	vxplr_ray_update();
	vxplr_inv_update(1);
	vxplr_cam_update_frustum();
}

/* Text pointers used during main loop for formatting values. */
static char *fmt_text[7];

/* Update text objects. */
static void vxmain_update_dynamic_text(VX_NO_ARG)
{
	static const char *dir_compass_texts[6] = { "East", "Up", "North", "West", "Down", "South" };
	static struct { double last_update, last_memory; } vxmain_memory_vals = { -1e+10, 0u };

	if (vxstate_vals.global_time - vxmain_memory_vals.last_update > 2.0) {
		vxmain_memory_vals.last_update = vxstate_vals.global_time;
		vxmain_memory_vals.last_memory = (VX_CAST(double, vxthr_get_memusage()) / 1024.0) / 1024.0;
	}

	vxtxt_obj_set_text(
		&vxmain_texts.game_info_txt,
		vxfmt_text("%d FPS | %d AVG | %d LOW (%.2fms)\n%s %s %s (" VXFMTPOS " " VXFMTPOS " " VXFMTPOS ")\n"
			"Velocity: %.2f %.2f %.2f\nYaw: %.1f Pitch: %.1f (%s, %s)\nFOV: %.1f Speed: %.1f\nFlying: %s Collision: %s",
			vxmain_counters.current, vxmain_counters.average, vxmain_counters.lowest, vxmain_counters.largest_frame_time * 1000.0,
			vxfmt_group_num(3, fmt_text + 0, "%f", vxplr_inst.pos.x),
			vxfmt_group_num(3, fmt_text + 1, "%f", vxplr_inst.pos.y),
			vxfmt_group_num(3, fmt_text + 2, "%f", vxplr_inst.pos.z),
			vxplr_inst.offset.x, vxplr_inst.offset.y, vxplr_inst.offset.z,
			vxplr_inst.vel.x, vxplr_inst.vel.y, vxplr_inst.vel.z,
			vxplr_cam.yaw, vxplr_cam.pitch, dir_compass_texts[vxplr_cam.yaw_dir], dir_compass_texts[vxplr_cam.pitch_dir],
			vxplr_cam.fov * VX_DEGREE_MULT, vxplr_inst.active_speed,
			VX_FMT_BOOL(!vxtg_toggles.gravity), VX_FMT_BOOL(vxtg_toggles.collision)
		), 0
	);
	vxtxt_obj_set_text(
		&vxmain_texts.world_info_txt,
		vxfmt_text("Regions: %zu/%zu Chunks: %s (Calls: %s)\nTris: %s Distance: %" PRIiMAX "\nMemory: %sMB\nGenerate: %s Borders: %s\nTime: %.1f Cycle: %.1f Day: %" PRIiMAX "",
			vxwld_info.rendered_regions_count, vxwld_regions.valid_count,
			vxfmt_group_num(0, fmt_text + 3, "%zu", vxwld_info.rendered_chunks_count),
			vxfmt_group_num(0, fmt_text + 4, "%zu",vxwld_info.draw_calls_count),
			vxfmt_group_num(0, fmt_text + 5, "%zu", vxwld_info.rendered_tris_count), vxwld_info.render_distance,
			vxfmt_group_num(3, fmt_text + 6, "%f", vxmain_memory_vals.last_memory),
			VX_FMT_BOOL(vxtg_toggles.generation_active), VX_FMT_BOOL(vxtg_toggles.chunk_borders),
			vxstate_vals.global_time, vxstate_vals.cycle_day_seconds, vxstate_vals.world_day_counter
		), 0
	);
}

/* Update values related to time. */
static void vxmain_time_update(VX_NO_ARG)
{
	vxstate_vals.global_time = glfwGetTime();
	vxstate_vals.frame_delta = vxstate_vals.global_time - vxmain_counters.prev_global_time;
	vxmain_counters.prev_global_time = vxstate_vals.global_time;

	vxstate_vals.ticked_delta_time = vxstate_vals.frame_delta * vxstate_vals.game_tick_speed;
	vxmain_counters.largest_frame_time = fmax(vxstate_vals.frame_delta, vxmain_counters.largest_frame_time);
	vxmain_counters.average_counter++;

	if ((vxstate_vals.cycle_day_seconds += vxstate_vals.ticked_delta_time) >= VX_SKY_DAY_SECONDS) {
		vxstate_vals.cycle_day_seconds = vxstate_vals.cycle_day_seconds - VX_SKY_DAY_SECONDS;
		++vxstate_vals.world_day_counter;
	}

	/* Window title and other text update. */
	if ((vxmain_counters.text_upd_waited += vxstate_vals.frame_delta) >= 0.1) {
		vxmain_counters.average = VX_CAST(int, VX_CAST(double, vxmain_counters.average_counter) / vxmain_counters.text_upd_waited);
		vxmain_counters.current = VX_CAST(int, 1.0 / vxstate_vals.frame_delta);
		vxmain_counters.lowest = VX_CAST(int, 1.0 / vxmain_counters.largest_frame_time);

		vxmain_update_dynamic_text();

		vxmain_counters.largest_frame_time = 0.0;
		vxmain_counters.text_upd_waited = 0.0;
		vxmain_counters.average_counter = 0;
	} 

	if ((vxmain_counters.events_waited_time += vxstate_vals.ticked_delta_time) > 0.05) {
		vxevent_apply_queue();
		vxmain_counters.events_waited_time = 0.0;
	}
}

/* Initialize and run a world instance. */
static void vxmain_begin(VX_NO_ARG)
{
	const double main_begin_time = glfwGetTime();
	vxlog_msg(VX_LOG_DEFAULT_BIT, "Main init complete");

	vxwld_init();
	vxplr_inv_init();
	vxsky_init();

	VX_EVENTS_HOOK_ADD(mouse_scroll, vxplr_inv_selected_scroll);
	VX_EVENTS_HOOK_ADD(window_resize, vxmain_aspect_changed);
	VX_EVENTS_HOOK_ADD(mouse_move, vxplr_cam_mouse_move);

	vxplr_ray_init();
	vxplr_cam_dirs_update();

#if VX_WINDOWS
	vxcb_window_resize(vxstate_vals.window_ptr, vxstate_vals.window_width, vxstate_vals.window_height);
#else
	vxcb_window_resize(
		vxstate_vals.window_ptr,
		VX_CAST(int, VX_CAST(float, vxstate_vals.window_width) * vxstate_vals.window_scale_x),
		VX_CAST(int, VX_CAST(float, vxstate_vals.window_height) * vxstate_vals.window_scale_y)
	);
#endif

	vxcb_window_resize(vxstate_vals.window_ptr, vxstate_vals.window_width, vxstate_vals.window_height);

	/* Text objects creation. */
	const uint8_t bg_shadow = vxen_txt_background | vxen_txt_shadow;
	const uint8_t bg_shadow_dbg = bg_shadow | vxen_txt_debug;

	char *info_fmt = vxfmt_text("%s\n%s\n%s\nSeed: %" PRIi64, gl.GetString(GL_VERSION), glfwGetVersionString(), gl.GetString(GL_RENDERER), vxwld_noise->seed);

	vxtxt_obj_init(&vxmain_texts.static_info_txt, VX_TEXT_LEFTX_CORNER, VX_TEXT_TOPY_CORNER, info_fmt, 0, bg_shadow_dbg, VX_TEXT_DEFAULT_FONT_SIZE, 1.0f);
	vxtxt_obj_init(&vxmain_texts.game_info_txt, VX_TEXT_LEFTX_CORNER, 0.0f, VX_NULL, 0, bg_shadow_dbg, VX_TEXT_DEFAULT_FONT_SIZE, 1.0f);
	vxtxt_obj_init(&vxmain_texts.world_info_txt, VX_TEXT_LEFTX_CORNER, 0.0f, VX_NULL, 0, bg_shadow_dbg, VX_TEXT_DEFAULT_FONT_SIZE, 1.0f);

	vxmain_update_dynamic_text();
	vxtxt_obj_move_y_relativeto(&vxmain_texts.game_info_txt, &vxmain_texts.static_info_txt, VX_TEXT_DEFAULT_OFFSET);
	vxtxt_obj_move_y_relativeto(&vxmain_texts.world_info_txt, &vxmain_texts.game_info_txt, VX_TEXT_DEFAULT_OFFSET);

	vxtxt_obj_init(&vxchat_text, VX_TEXT_LEFTX_CORNER, -0.12f, VX_NULL, 0, bg_shadow, VX_TEXT_DEFAULT_FONT_SIZE, 0.0f);
	vxtxt_obj_init(&vxcmd_text, VX_TEXT_LEFTX_CORNER, -0.74f, VX_NULL, 0, bg_shadow | vxen_txt_bg_full_width, VX_TEXT_DEFAULT_FONT_SIZE, 0.0f);

	vxtg_toggles.first_generate = 1;
	vxtg_toggles.loop_active = !vxmain_args.exit_immediate;

	const double enter_time = glfwGetTime();
	vxlog_free(VX_LOG_DEFAULT_BIT, vxfmt_text("World init complete (%.2fms), seed: %" PRIi64, (enter_time - main_begin_time) * 1000.0, vxwld_noise->seed));

	/* Main loop. */
	while (vxtg_toggles.loop_active) {
		glfwPollEvents();

                vxmain_time_update();
		vxplr_move_logic();
		vxmain_update_ubos();

		vxwld_observe();
		vxctx_draw_all();

		glfwSwapBuffers(vxstate_vals.window_ptr);
	}

	vxlog_free(VX_LOG_DEFAULT_BIT, vxfmt_text("Exiting world (ran for %.5fs)", glfwGetTime() - enter_time));

	vxwld_destroy();
	vxevent_destroy(0);
	for (size_t i = 0u; i < sizeof fmt_text / sizeof *fmt_text; ++i) VX_FREE(fmt_text[i]);

	glfwSetInputMode(vxstate_vals.window_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	vxtg_toggles.loop_active = 0;
}

#if defined(_MSC_VER)
#define VX_NO_RET __declspec(noreturn)
#else
#define VX_NO_RET __attribute__((noreturn))
#endif

/* Help text about existing command arguments. */
VX_NO_RET static void vxmain_display_help(char *argv_start)
{
	fprintf(stdout, "Usage: %s [options]\nOptions:\n", argv_start);
	fprintf(stdout, "  --help        Displays this message.\n");
	fprintf(stdout, "  --version     Displays version and license info.\n");
	fprintf(stdout, "  --clear-log   Clears the active log file.\n");
	fprintf(stdout, "  --log-file=   Writes all in-game logs to the given file.\n");
	fprintf(stdout, "  --verbose=    Specifies the amount of debug info to log to the terminal.\n");
	fprintf(stdout, "  --threads=    Specifies the number of threads to use. Must be above 1.\n");
	fprintf(stdout, "  -x            Forces using X11 instead of Wayland on supported systems.\n");
	fprintf(stdout, "  -e            Skips the main rendering loop and immediately exits.\n");
	fprintf(stdout, "  -n            Disables logging to a file. Has precedence over --log-file.\n");
	fprintf(stdout, "  -t            Disables logging to the terminal.\n");
	fprintf(stdout, "  -N            Disables logging completely.\n");
	exit(0);
}
/* Version text for command arguments handling. */
VX_NO_RET static void vxmain_display_version(VX_NO_ARG)
{
	fprintf(stdout, VX_GAME_TITLE "\n" VX_COPYRIGHT_TITLE);
	fprintf(stdout, "\nThis is free software; you can redistribute it under certain conditions. It comes WITHOUT ANY WARRANTY.");
	fprintf(stdout, "\nFor details, see the GNU General Public License.\n");
	exit(0);
}
/* Apply settings from given argument character (e.g. -abc gives args a, b and c). */
static int vxmain_handle_char_arg(char arg_char)
{
	switch (arg_char) {
		default: return 0;
		case 'e':
			vxmain_args.exit_immediate = 1;
			break;
		case 'x':
			if (glfwPlatformSupported(GLFW_PLATFORM_X11)) glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
			else { fprintf(stderr, "X11 is not supported on this machine.\n"); exit(-1); }
			break;
		case 't':
			vxlog_info.terminal_logging = 0;
			break;
		case 'N':
			vxlog_info.terminal_logging = 0;
			vxlog_info.file_logging = 0;
			break;
		case 'n':
			vxlog_info.file_logging = 0;
			break;
	}

	return 1;
}

static FILE *vxmain_fopen(const char *VX_RESTRICT filename, const char *VX_RESTRICT mode)
{
#if defined (_MSC_VER)
	FILE *streamptr;
	const errno_t e = fopen_s(&streamptr, filename, mode);
	return e ? NULL : streamptr;
#else
	return fopen(filename, mode);
#endif
}

/* Determine options from command-line arguments. */
static void vxmain_get_argvs(int argc, char **argv)
{
	const char *log_file_name = VX_NULL;
	const char *log_mode = "a+";

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		const char *err_type = "Unknown";

		const int has_first_dash = *arg == '-';
		const int is_double_dash = has_first_dash && arg[1] == '-';

		if (!has_first_dash) goto unknown_argument;
	
		/* Skip dashes to begin parsing actual command after them. */
		arg += is_double_dash + 1;
		if (!arg) goto invalid_argument;

		if (is_double_dash) {
			#define IS_ARG(base_name) strncmp(arg, base_name, sizeof base_name - 1u) == 0
			if (IS_ARG("help")) { vxmain_display_help(*argv); }
			else if (IS_ARG("version")) { vxmain_display_version(); }
			else if (IS_ARG("log-file=")) { log_file_name = arg + sizeof "log-file"; }
			else if (IS_ARG("verbose=")) { vxlog_info.verbosity = VX_CAST(int, strtol(arg + sizeof "verbose=" - 1, VX_NULL, 0)); }
			else if (IS_ARG("clear-log")) { log_mode = "w+"; }
			else if (IS_ARG("threads=")) {
				vxstate_vals.available_threads = VX_CAST(int, strtol(arg + sizeof "threads=" - 1, VX_NULL, 0));
				if (vxstate_vals.available_threads <= 1) goto invalid_argument;
			}
			else goto unknown_argument;
		} else while (*arg) if (!vxmain_handle_char_arg(*arg++)) goto unknown_argument;
		continue;
	invalid_argument:
		err_type = "Invalid";
	unknown_argument:
		fprintf(stderr, "%s option '%s'. Use --help to see available commands.\n", err_type, arg);
		exit(0);
	}

	/* Use user-specified log file if there is one, otherwise the default if file logging is enabled. */
	char *concat_text = VX_NULL;
	#define VX_FULL_PATH(suffix) (concat_text = vxfmt_concat_allocd(0, &vxfile_exec_dir, suffix))

	if (log_file_name && vxlog_info.file_logging && !(vxlog_info.vxlog_stream = vxmain_fopen(VX_FULL_PATH(log_file_name), log_mode))) {
		fprintf(stderr, "Creating/opening log file '%s' failed.\n", log_file_name);
	}
	else if (!log_file_name && vxlog_info.file_logging && !(vxlog_info.vxlog_stream = vxmain_fopen(VX_FULL_PATH("default_log.txt"), log_mode))) {
		fprintf(stderr, "Creating/opening default log file failed.\n");
	}

	/* Make sure to disable file logging if creation failed. */
	if (!vxlog_info.vxlog_stream) vxlog_info.file_logging = 0;
	if (concat_text) VX_FREE(concat_text);
}

int main(int argc, char **argv)
{
	if (!vxfile_parent_from_path(vxfile_get_exec_path(&vxfile_exec_dir, *argv))) VX_ABORT("Could not find executable path.");
	vxmain_get_argvs(argc, argv);

	time_t cur_time = time(VX_NULL);
	struct tm cur_date = *localtime(&cur_time);
	vxlog_free(
		VX_LOG_NOTIME_BIT | VX_LOG_NOSEPNL_BIT,
		vxfmt_text("%02d/%02d/%d %02d:%02d:%02d ", cur_date.tm_mday, cur_date.tm_mon + 1, cur_date.tm_year + 1900, cur_date.tm_hour, cur_date.tm_min, cur_date.tm_sec)
	);
	vxlog_msg(VX_LOG_NOTIME_BIT, ">>> " VX_GAME_TITLE " - " VX_COPYRIGHT_TITLE " <<<");

	if (glfwInit() == GLFW_FALSE) VX_ABORT("GLFW initialization failed.");
	glfwSetErrorCallback(vxlog_glfw_err);

	if (!vxstate_init()) VX_ABORT("Could not get display information.");

	glfwWindowHint(GLFW_POSITION_X, vxstate_vals.window_xpos);
	glfwWindowHint(GLFW_POSITION_Y, vxstate_vals.window_ypos);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#define VX_GLVER(major, minor) ((major << 4) + minor)
	static const uint8_t gl_versions[] = {
		VX_GLVER(4, 6), VX_GLVER(4, 5), VX_GLVER(4, 4),
		VX_GLVER(4, 3), VX_GLVER(4, 2), VX_GLVER(4, 1),
		VX_GLVER(4, 0), VX_GLVER(3, 3)
	};
	
#if defined(__APPLE__) && defined(__MACH__)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

	for (size_t i = 0; i < sizeof gl_versions; ++i) {
		const int major = gl_versions[i] >> 4;
		const int minor = gl_versions[i] & 0xF;

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);

		if ((vxstate_vals.window_ptr = glfwCreateWindow(
			vxstate_vals.window_width,
			vxstate_vals.window_height,
			VX_GAME_TITLE,
			VX_NULL, VX_NULL)
		)) {
			vxstate_vals.ogl_major = major;
			vxstate_vals.ogl_minor = minor;
			break;
		}
	}

	if (!vxstate_vals.window_ptr) VX_ABORT("Window creation failed.");

        glfwMakeContextCurrent(vxstate_vals.window_ptr);
	
	glfwSetWindowSizeLimits(vxstate_vals.window_ptr, 300, 200, GLFW_DONT_CARE, GLFW_DONT_CARE);
	glfwGetWindowContentScale(vxstate_vals.window_ptr, &vxstate_vals.window_scale_x, &vxstate_vals.window_scale_y);
#if VX_WINDOWS
	glfwSetWindowSize(
		vxstate_vals.window_ptr,
		vxstate_vals.window_width = VX_CAST(int, VX_CAST(float, vxstate_vals.window_width) * vxstate_vals.window_scale_x),
		vxstate_vals.window_height = VX_CAST(int, VX_CAST(float, vxstate_vals.window_height) * vxstate_vals.window_scale_y)
	);
#endif
	glfwSwapInterval(1);

	vxgl_init_ogl(glfwGetProcAddress);
	vxlog_free(VX_LOG_DEFAULT_BIT, vxfmt_text("Libraries loaded - %s | %s", gl.GetString(GL_VERSION), glfwGetVersionString()));

	if (gl.DebugMessageCallback) {
		vxlog_msg(VX_LOG_DEFAULT_BIT, "OpenGL debug messages enabled");
		gl.Enable(GL_DEBUG_OUTPUT);
		gl.Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		gl.DebugMessageCallback(vxlog_ogl_debugout, VX_NULL);
	}

	vxelm_load();
	vxsd_init_all();

	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl.Enable(GL_DEPTH_TEST);
	gl.DepthFunc(GL_LEQUAL);

	gl.Enable(GL_CULL_FACE);
	gl.CullFace(GL_BACK);

	gl.Enable(GL_PROGRAM_POINT_SIZE);

	vxevent_init_all();
	vxtxt_mgr_init();

	vxmain_begin();

	VX_FREE(vxfile_exec_dir);

	vxtxt_mgr_destroy();
	vxevent_destroy(1);
	vxctx_destroy_all();
	vxsd_destroy();

	vxlog_msg(VX_LOG_DEFAULT_BIT, "(Exit)");

	if (vxlog_info.vxlog_stream) {
		fprintf(VX_CAST(FILE *, vxlog_info.vxlog_stream), "\n");
		fclose(VX_CAST(FILE *, vxlog_info.vxlog_stream));
	}

	glfwTerminate();
}
