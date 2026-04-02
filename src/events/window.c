#include "events/window.h"
#include "events/events.h"

#include "directives/dcast.h"
#include "directives/dword.h"

#include "graphics/glfuncs.h"
#include "graphics/glfw.h"

#include "values/state.h"

#define VX_UNUSED(x) do { (void)(x); } while (0)

void vxcb_window_resize(GLFWwindow *window, int width, int height)
{
	VX_UNUSED(window);
	if (vxtg_toggles.iconified) return;

	gl.Viewport(0, 0, width, height);

	vxstate_vals.window_width = width;
	vxstate_vals.window_height = height;

	vxstate_vals.aspect = VX_CAST(float, vxstate_vals.window_width) / VX_CAST(float, vxstate_vals.window_height);

	VX_EVENTS_HOOK_EXECUTE(window_resize, width, height);
}

void vxcb_window_move(GLFWwindow *window, int x, int y)
{
	VX_UNUSED(window);
	vxstate_vals.window_xpos = x;
	vxstate_vals.window_ypos = y;
}

void vxcb_window_icond(GLFWwindow *window, int iconified)
{
	VX_UNUSED(window);
	vxtg_toggles.iconified = VX_CAST(unsigned char, iconified);
	glfwSwapInterval(vxtg_toggles.iconified ? vxstate_vals.screen_refresh_rate : VX_CAST(int, vxtg_toggles.synced_fps));
}

void vxcb_window_close(GLFWwindow *window)
{
	VX_UNUSED(window);
	vxtg_toggles.loop_active = 0;
}


void vxcb_toggle_fscreen(VX_NO_ARG)
{
	vxtg_toggles.window_focus_changed = 1;

	/* Turn off fullscreen. */
	if (!VX_FLIP(vxtg_toggles.fullscreen)) {
		glfwSetWindowMonitor(
			vxstate_vals.window_ptr,
			VX_NULL, /* Windowed mode, no monitor specified. */
			vxstate_vals.default_window_xpos,
			vxstate_vals.default_window_ypos,
			vxstate_vals.default_window_width,
			vxstate_vals.default_window_height, 0
		);
		return;
	}

	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *mode = glfwGetVideoMode(monitor);
	glfwSetWindowMonitor(vxstate_vals.window_ptr, monitor, 0, 0, mode->width, mode->height, 0);
}
