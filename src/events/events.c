#include "events/events.h"
#include "events/window.h"
#include "events/mouse.h"
#include "events/keys.h"

#include "directives/dcast.h"
#include "directives/dfree.h"

#include "graphics/glfw.h"

#include "values/state.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct vxstruct_event_hooks_list vxcb_hooks;
static int (**vxloc_events)(VX_NO_ARG);
static size_t vxloc_events_count;

void vxevent_init_all(VX_NO_ARG)
{
	GLFWwindow *wptr = vxstate_vals.window_ptr;
	
	glfwSetFramebufferSizeCallback(wptr, vxcb_window_resize);
	glfwSetWindowIconifyCallback(wptr, vxcb_window_icond);
	glfwSetWindowCloseCallback(wptr, vxcb_window_close);
	glfwSetWindowPosCallback(wptr, vxcb_window_move);

	glfwSetMouseButtonCallback(wptr, vxcb_mouse_click);
	glfwSetCursorPosCallback(wptr, vxcb_mouse_move);
	glfwSetScrollCallback(wptr, vxcb_mouse_scroll);

	glfwSetCharCallback(wptr, vxkey_char_point);
	glfwSetKeyCallback(wptr, vxkey_enter);
}

void vxevent_hook_add_impl(struct vxstruct_event_hook_base *hook, void (*to_call)(VX_NO_ARG))
{
	void *hooks_ptr = realloc(hook->hooks, sizeof to_call * VX_CAST(size_t, ++hook->count));
	if (!hooks_ptr) return;

	hook->hooks = VX_CAST(void (**)(VX_NO_ARG), hooks_ptr);
	hook->hooks[hook->count - 1] = to_call;
}

void vxevent_apply_queue(VX_NO_ARG)
{
	/* Move events queue backwards to overwrite completed events. */
	for (size_t i = 0u; i < vxloc_events_count; ++i) {
		if (vxloc_events[i]() == 0) continue;
		
		if (!--vxloc_events_count) {
			VX_FREE(vxloc_events);
			break;
		}

		memmove(vxloc_events + i, vxloc_events + i + 1, sizeof *vxloc_events * (vxloc_events_count - i));
	}
}

int vxevent_add(int (*event)(VX_NO_ARG))
{
	void *rlc_res = realloc(vxloc_events, sizeof *vxloc_events * ++vxloc_events_count);

	if (!rlc_res) return 0;
	else if (rlc_res != vxloc_events) vxloc_events = VX_CAST(int (**)(VX_NO_ARG), rlc_res);

	vxloc_events[vxloc_events_count - 1] = event;
	return 1;
}

void vxevent_destroy(int persistent)
{
	if (vxloc_events_count) {
		VX_FREE(vxloc_events);
		vxloc_events_count = 0u;
	}

	if (!persistent) return;

	struct vxstruct_event_hook_base *hooks_ptr = VX_REINT_CAST(struct vxstruct_event_hook_base *, &vxcb_hooks);
	for (size_t i = 0u; i < (sizeof vxcb_hooks / sizeof *hooks_ptr); ++i) {
		if (hooks_ptr[i].count) VX_FREE(hooks_ptr[i].hooks);
		hooks_ptr[i].count = 0;
	}
}
