#include "events/mouse.h"
#include "events/events.h"

#include "graphics/glfw.h"

#include "values/state.h"

#define VX_UNUSED(x) do { (void)(x); } while (0)

void vxcb_mouse_click(GLFWwindow *window, int button, int action, int mods)
{
	VX_UNUSED(window);
	if (mods != -1) glfwSetInputMode(vxstate_vals.window_ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (vxtg_toggles.chatting) return;

	VX_EVENTS_HOOK_EXECUTE(mouse_click, button, action);
}

void vxcb_mouse_move(GLFWwindow *window, double xpos, double ypos)
{
	VX_UNUSED(window);
	if (vxtg_toggles.chatting) return;

	static double prev_xpos = 0.0, prev_ypos = 0.0;

	/* Avoid large jumps when regaining focus. */
	if (vxtg_toggles.window_focus_changed) {
		--vxtg_toggles.window_focus_changed;
		prev_xpos = xpos;
		prev_ypos = ypos;
	}

	VX_EVENTS_HOOK_EXECUTE(mouse_move, xpos, ypos, xpos - prev_xpos, prev_ypos - ypos);

	prev_xpos = xpos;
	prev_ypos = ypos;
}

void vxcb_mouse_scroll(GLFWwindow *window, double x_offset, double y_offset)
{
	VX_UNUSED(window);
	if (vxtg_toggles.chatting) return;
	VX_EVENTS_HOOK_EXECUTE(mouse_scroll, x_offset, y_offset);
}
