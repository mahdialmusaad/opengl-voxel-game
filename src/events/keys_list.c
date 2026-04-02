#include "events/keys.h"
#include "events/screenshot.h"
#include "events/commands.h"
#include "events/window.h"
#include "events/mouse.h"

#include "directives/dmath.h"

#include "graphics/glenum.h"
#include "graphics/glfuncs.h"
#include "graphics/glfw.h"

#include "player/inventory.h"
#include "player/movement.h"
#include "player/camera.h"

#include "shaders/loader.h"

#include "text/text_mgr.h"

#include "values/state.h"

#include "world/map.h"

#include "io/logs.h"

/* Functions of key inputs. */

#define INPUT_FUNC(name) static void kcb_##name(VX_NO_ARG)

INPUT_FUNC(command_chat) { vxcmd_begin(0); }
INPUT_FUNC(normal_chat) { vxcmd_begin(1); }
INPUT_FUNC(close_game) { vxtg_toggles.loop_active = 0; }

INPUT_FUNC(toggle_sync) { glfwSwapInterval(VX_FLIP(vxtg_toggles.synced_fps)); }
INPUT_FUNC(toggle_inv) { vxplr_inv_toggle(!vxtg_toggles.inventory_open); }
INPUT_FUNC(toggle_fog) { VX_FLIP(vxtg_toggles.hide_fog); }
INPUT_FUNC(toggle_fly) { VX_FLIP(vxtg_toggles.gravity); }
INPUT_FUNC(toggle_collide) { VX_FLIP(vxtg_toggles.collision); }
INPUT_FUNC(toggle_gen) { VX_FLIP(vxtg_toggles.generation_active); }

INPUT_FUNC(inc_rnd_dist) { vxwld_change_rdist(vxwld_info.render_distance + 1); }
INPUT_FUNC(dec_rnd_dist) { vxwld_change_rdist(vxwld_info.render_distance - 1); }
INPUT_FUNC(inc_speed) { vxplr_inst.base_speed += 1.0; }
INPUT_FUNC(dec_speed) { vxplr_inst.base_speed -= 1.0; }
INPUT_FUNC(inc_fov) { vxplr_cam_update_fov(vxplr_cam.fov + VX_RADIAN_MULT); }
INPUT_FUNC(dec_fov) { vxplr_cam_update_fov(vxplr_cam.fov - VX_RADIAN_MULT); }

INPUT_FUNC(toggle_polygon_mode) { gl.PolygonMode(GL_FRONT_AND_BACK, VX_FLIP(vxtg_toggles.wireframe) ? GL_LINE : GL_FILL); }
INPUT_FUNC(reload_shaders) {
	vxlog_msg(VX_LOG_WARNING_BIT, "Reloading elements...");
	vxwld_pause_threads(1);
	vxsd_destroy();
	vxelm_destroy();
	vxelm_load();
	vxsd_init_all();
	vxtxt_mgr_init();
	vxwld_setup_uniform();
	vxwld_queue_all_remesh();
	vxwld_resume_threads();
	vxlog_msg(VX_LOG_WARNING_BIT, "Finished reloading.");
}
INPUT_FUNC(toggle_borders) { VX_FLIP(vxtg_toggles.chunk_borders); }
INPUT_FUNC(simulate_mouse) { vxcb_mouse_click(vxstate_vals.window_ptr, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, -1); }

INPUT_FUNC(toggle_gui) { VX_FLIP(vxtg_toggles.show_any_gui); }
INPUT_FUNC(take_screenshot) { vxshot_take_screenshot(); }
INPUT_FUNC(free_mouse) { glfwSetInputMode(vxstate_vals.window_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL); vxtg_toggles.window_focus_changed = 1; }
INPUT_FUNC(toggle_dbg_text) { VX_FLIP(vxtg_toggles.debug_text); }
INPUT_FUNC(toggle_fullscreen) { vxcb_toggle_fscreen(); }

static const int press_bit = GLFW_PRESS;
static const int bypass_chat = 1 << 6;

const struct vxkey_input_event_obj vxkey_actions[] = {
/* Standard inputs. */
/* Movement keys are checked per frame elsewhere. */
{
	GLFW_KEY_SLASH,
	press_bit,
	kcb_command_chat
},
{
	GLFW_KEY_T,
	press_bit,
	kcb_normal_chat
},
{
	GLFW_KEY_ESCAPE,
	press_bit,
	kcb_close_game
},
/* Toggle inputs. */
{
	GLFW_KEY_X,
	press_bit,
	kcb_toggle_sync
},
{
	GLFW_KEY_E,
	press_bit,
	kcb_toggle_inv
},
{
	GLFW_KEY_F,
	press_bit,
	kcb_toggle_fog
},
{
	GLFW_KEY_C,
	press_bit,
	kcb_toggle_fly
},
{
	GLFW_KEY_N,
	press_bit,
	kcb_toggle_collide
},
{
	GLFW_KEY_V,
	press_bit,
	kcb_toggle_gen
},
/* Value inputs: */
{ 
	GLFW_KEY_LEFT_BRACKET,
	press_bit,
	kcb_inc_rnd_dist
},
{
	GLFW_KEY_RIGHT_BRACKET,
	press_bit,
	kcb_dec_rnd_dist
},
{
	GLFW_KEY_COMMA,
	press_bit,
	kcb_inc_speed
},
{
	GLFW_KEY_PERIOD,
	press_bit,
	kcb_dec_speed
},
{
	GLFW_KEY_I,
	press_bit,
	kcb_inc_fov
},
{
	GLFW_KEY_O,
	press_bit,
	kcb_dec_fov
},
/* Debug inputs. */
{
	GLFW_KEY_Z,
	press_bit,
	kcb_toggle_polygon_mode
},
{
	GLFW_KEY_R,
	press_bit,
	kcb_reload_shaders
},
{
	GLFW_KEY_J,
	press_bit,
	kcb_toggle_borders
},
{
	GLFW_KEY_K,
	press_bit,
	kcb_simulate_mouse
},
/* Function key inputs. */
{
	GLFW_KEY_F1,
	press_bit | bypass_chat,
	kcb_toggle_gui
},
{
	GLFW_KEY_F2,
	press_bit | bypass_chat,
	kcb_take_screenshot
},
{
	GLFW_KEY_F3,
	press_bit | bypass_chat,
	kcb_free_mouse
},
{
	GLFW_KEY_F4,
	press_bit | bypass_chat,
	kcb_toggle_dbg_text
},
{
	GLFW_KEY_F11,
	press_bit | bypass_chat,
	kcb_toggle_fullscreen
}
};

const unsigned short vxkey_actions_count = sizeof vxkey_actions / sizeof *vxkey_actions;
