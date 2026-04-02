#pragma once
#ifndef SOURCE_EVENTS_KEYS_VXL_HDR
#define SOURCE_EVENTS_KEYS_VXL_HDR
/* Key events handler. */

#include "directives/dextern.h"

struct GLFWwindow;

/* Input action and key data. */
struct vxkey_input_event_obj
{
	int key, accept_bits;
	void (*action)(VX_NO_ARG);
};
/* List of all key actions. */
extern const struct vxkey_input_event_obj vxkey_actions[];
/* Number of key actions. */
extern const unsigned short vxkey_actions_count;
/* Current key states. */
extern unsigned char vxkey_states[];

VX_C_START

/* Key input event. */
void vxkey_enter(struct GLFWwindow *window, int key, int scancode, int action, int mods);
/* Entered character codepoint event. */
void vxkey_char_point(struct GLFWwindow *window, unsigned int codepoint);

VX_C_END

#endif
