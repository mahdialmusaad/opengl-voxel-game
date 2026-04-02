#pragma once
#ifndef SOURCE_EVENTS_MOUSE_VXL_HDR
#define SOURCE_EVENTS_MOUSE_VXL_HDR
/* Mouse events handler. */

#include "directives/dextern.h"

struct GLFWwindow;

VX_C_START

/* Mouse click event. */
void vxcb_mouse_click(struct GLFWwindow *window, int button, int action, int mods);
/* Mouse move event. */
void vxcb_mouse_move(struct GLFWwindow *window, double xpos, double ypos);
/* Mouse scroll event. */
void vxcb_mouse_scroll(struct GLFWwindow *window, double x_offset, double y_offset);

VX_C_END

#endif
