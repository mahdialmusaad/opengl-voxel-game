#include "values/state.h"

#include "directives/dcast.h"

#include "graphics/glfw.h"

#include "utils/thread.h"
#include <GLFW/glfw3.h>

struct vxstruct_global_state vxstate_vals;
struct vxstruct_global_toggles vxtg_toggles;

int vxstate_init(VX_NO_ARG)
{
	/* Get display information. */
	const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	if (!mode) return 0;

	/* State values. */
	
	vxstate_vals.screen_refresh_rate = mode->refreshRate;
	vxstate_vals.screen_height = mode->height;
	vxstate_vals.screen_width = mode->width;
	
	vxstate_vals.window_width = vxstate_vals.screen_width / 3;
	vxstate_vals.window_height = vxstate_vals.screen_height / 3;
	vxstate_vals.aspect = VX_CAST(float, vxstate_vals.window_width) / VX_CAST(float, vxstate_vals.window_height);
	
	vxstate_vals.window_xpos = (vxstate_vals.screen_width / 2) - (vxstate_vals.window_width / 2);
	vxstate_vals.window_ypos = (vxstate_vals.screen_height / 2) - (vxstate_vals.window_height / 2);
	
	vxstate_vals.default_window_xpos = vxstate_vals.window_xpos;
	vxstate_vals.default_window_ypos = vxstate_vals.window_ypos;
	vxstate_vals.default_window_width = vxstate_vals.window_width;
	vxstate_vals.default_window_height = vxstate_vals.window_height;

	vxstate_vals.frame_update_interval = 1;
	vxstate_vals.game_tick_speed = 1.0;

	if (!vxstate_vals.available_threads) {
		vxstate_vals.available_threads = vxthr_get_threads();
		if (vxstate_vals.available_threads < 2) vxstate_vals.available_threads = 2;
	}

	/* Toggle values. */

	vxtg_toggles.first_generate = 1u;
	vxtg_toggles.in_world_instance = 1u;

	vxtg_toggles.window_focus_changed = 5u;
	vxtg_toggles.synced_fps = 1u;

	vxtg_toggles.show_any_gui = 1u;
	vxtg_toggles.debug_text = 1u;
	vxtg_toggles.generation_active = 1u;
	
	return 1;
}
