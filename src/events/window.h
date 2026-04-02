#pragma once
#ifndef SOURCE_EVENTS_WINDOW_VXL_HDR
#define SOURCE_EVENTS_WINDOW_VXL_HDR
/* General window events handler. */

#include "directives/dextern.h"

struct GLFWwindow;

VX_C_START

/* Game window resize event. */
void vxcb_window_resize(struct GLFWwindow *window, int width, int height);
/* Game window moved event. */
void vxcb_window_move(struct GLFWwindow *window, int x, int y);
/* Game window iconification (minimized) event. */
void vxcb_window_icond(struct GLFWwindow *window, int iconified);
/* Game window close event. */
void vxcb_window_close(struct GLFWwindow *window);

/* Toggle fullscreen. */
void vxcb_toggle_fscreen(VX_NO_ARG);

VX_C_END

#endif
