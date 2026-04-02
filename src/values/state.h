#pragma once
#ifndef SOURCE_VALUES_STATE_VXL_HDR
#define SOURCE_VALUES_STATE_VXL_HDR
/* Game state. */

#include "directives/dextern.h"

#include <stdint.h>

struct GLFWwindow;

/* Invert value. */
#define VX_FLIP(x) (x = !x)

/* Game global state object. */
extern struct vxstruct_global_state
{
	/* Pointer to the GLFW window object. */
	struct GLFWwindow *window_ptr;
	
	/* Default and current window XY position. */
	int window_xpos, window_ypos, default_window_xpos, default_window_ypos;
	/* Default and current window XY size. */
	int window_width, window_height, default_window_width, default_window_height;
	/* Primary monitor dimensions. */
	int screen_width, screen_height;
	/* Number of swap intervals. */
	int frame_update_interval;
	/* Primary monitor refresh rate. */
	int screen_refresh_rate;
	/* Current window aspect ratio (width / height). */
	float aspect;
	/* Window scaling factor. */
	float window_scale_x, window_scale_y;

	/* Alpha of sunrise/sunset orange colour. */
	float twilight_colour_trnsp;
	/* Available threads on system. */
	int available_threads;

	float unused;

	/* Game time multiplier. */
	double game_tick_speed;
	/* Current frame time. */
	double frame_delta;
	/* Frame time * tick speed. */
	double ticked_delta_time;
	/* GLFW time, updated per frame. */
	double global_time;
	/* No. of seconds that have passed in the current day. */
	double cycle_day_seconds;
	
	intmax_t world_day_counter;
	uintmax_t game_frame_counter;
} vxstate_vals;

/* Global value toggles object. */
extern struct vxstruct_global_toggles
{
	unsigned char first_generate;
	unsigned char in_world_instance;

	unsigned char chatting;
	unsigned char not_command_chat;
	unsigned char ignore_first;

	unsigned char window_focus_changed;
	unsigned char fullscreen;
	unsigned char synced_fps;
	unsigned char iconified;
	
	unsigned char show_any_gui;
	unsigned char debug_text;
	
	unsigned char generation_active;
	unsigned char loop_active;

	unsigned char inventory_open;
	unsigned char collision;
	unsigned char grounded;
	unsigned char gravity;
	
	unsigned char chunk_borders;
	unsigned char wireframe;
	unsigned char hide_fog;
} vxtg_toggles;

/* Initialize game global. */
VX_C_FUNC int vxstate_init(VX_NO_ARG);

#endif
