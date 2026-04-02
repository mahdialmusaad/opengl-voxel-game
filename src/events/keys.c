#include "events/keys.h"
#include "events/commands.h"
#include "events/chat.h"

#include "directives/dcast.h"

#include "player/inventory.h"

#include "text/text_obj.h"

#include "graphics/glfw.h"

#include "values/state.h"

#include <string.h>

unsigned char vxkey_states[GLFW_KEY_LAST];

#define VX_UNUSED(x) do { (void)(x); } while (0)

void vxkey_char_point(GLFWwindow *window, unsigned int codepoint)
{
	VX_UNUSED(window);
	if (!vxtg_toggles.chatting || !vxtg_toggles.in_world_instance) return;
	if (vxtg_toggles.ignore_first) {
		vxtg_toggles.ignore_first = 0;
		return;
	}
	
	/* Add inputted char into command text. */
	const char character[2] = { VX_CAST(char, codepoint), '\0' };
	vxcmd_add_text(character);
}

void vxkey_enter(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	VX_UNUSED(window); VX_UNUSED(scancode); VX_UNUSED(mods);
	if (action == GLFW_REPEAT) return;

	vxkey_states[key] = VX_CAST(unsigned char, action);

	if (vxtg_toggles.in_world_instance) {
		if (vxtg_toggles.chatting) {
			if (action == GLFW_RELEASE) return;

			if (key == GLFW_KEY_ENTER) {
				vxcmd_execute();
				vxcmd_hide_texts(!VX_TEXT_EMPTY(vxchat_text));
			}
			else if (key == GLFW_KEY_ESCAPE) {
				vxcmd_hide_texts(0);
				return;
			}
			else if (key == GLFW_KEY_BACKSPACE) {
				if (VX_TEXT_EMPTY(vxcmd_text)) return;
				vxcmd_text.text[strlen(vxcmd_text.text) - 1] = '\0';
				vxtxt_obj_text_recalc(&vxcmd_text);
			}
		}
		/* Select the corresponding inventory slot if a number is pressed. */
		else if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
			vxplr_inv_selected(key - GLFW_KEY_1);
			return;
		}
	}

	/* Input types. */
	const int start_shift = GLFW_REPEAT;
	const int no_ctrl = 1 << (start_shift + 1), no_alt = 1 << (start_shift + 2), no_shift = 1 << (start_shift + 3);
	const int bypass_chat = 1 << (start_shift + 4);

	/* Check if the pressed key corresponds to an input action. */
	const struct vxkey_input_event_obj *event_end_ptr = vxkey_actions + vxkey_actions_count, *cur_event_ptr = vxkey_actions;
	for (; cur_event_ptr != event_end_ptr; ++cur_event_ptr) if (cur_event_ptr->key == key) break;

	if (cur_event_ptr != event_end_ptr) {
		/* Check if input action disallows execution with specific modifiers or states. */
		if ((cur_event_ptr->accept_bits & no_ctrl) && (mods & GLFW_MOD_CONTROL)) return;
		if ((cur_event_ptr->accept_bits & no_alt) && (mods & GLFW_MOD_ALT)) return;
		if ((cur_event_ptr->accept_bits & no_shift) && (mods & GLFW_MOD_SHIFT)) return;
		if (!(cur_event_ptr->accept_bits & bypass_chat) && vxtg_toggles.chatting) return;
		/* Run if the action type matches. */
		if (action & cur_event_ptr->accept_bits) cur_event_ptr->action();
	}
}
