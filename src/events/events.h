#pragma once
#ifndef SOURCE_EVENTS_CALLBACKS_VXL_HDR
#define SOURCE_EVENTS_CALLBACKS_VXL_HDR
/* Main callbacks declarations. */

#include "directives/dextern.h"
#include "directives/dcast.h"

#include <stddef.h>

struct vxstruct_event_hook_base
{
	void (**hooks)(VX_NO_ARG);
	size_t count;
};
/* Callbacks hook objects. */
struct vxstruct_event_hooks_list
{
	#define VX_HOOK_INST(name, ...) struct { void (**hooks)(__VA_ARGS__); size_t count; } name
	
	VX_HOOK_INST(mouse_move, double x, double y, double delta_x, double delta_y);
	VX_HOOK_INST(mouse_scroll, double delta_x, double delta_y);
	VX_HOOK_INST(window_resize, int width, int height);
	VX_HOOK_INST(mouse_click, int button, int action);
	VX_HOOK_INST(player_moved, VX_NO_ARG);
	
	#undef VX_HOOK_INST
};
/* Functions to call as well after callbacks events. */
extern struct vxstruct_event_hooks_list vxcb_hooks;

#if defined(__clang__)
/* Add a hook to a callback event function. */
# define VX_EVENTS_HOOK_ADD(name, func)\
_Pragma("clang diagnostic push")\
_Pragma("clang diagnostic ignored \"-Wcast-function-type-strict\"")\
vxevent_hook_add_impl(VX_REINT_CAST(struct vxstruct_event_hook_base *, &vxcb_hooks.name), VX_REINT_CAST(void (*)(VX_NO_ARG), func))\
_Pragma("clang diagnostic pop")
#else
/* Add a hook to a callback event function. */
# define VX_EVENTS_HOOK_ADD(name, func)\
vxevent_hook_add_impl(VX_REINT_CAST(struct vxstruct_event_hook_base *, &vxcb_hooks.name), VX_REINT_CAST(void (*)(VX_NO_ARG), func))
#endif

/* Execute all functions hooked to the given callback event with the relevant arguments. */
# define VX_EVENTS_HOOK_EXECUTE(name, ...) for (size_t i = 0u; i < vxcb_hooks.name.count; ++i) vxcb_hooks.name.hooks[i](__VA_ARGS__)

#define VX_GAME_TICK_MIN (-1000.0)
#define VX_GAME_TICK_MAX (1000.0)

VX_C_START

/* Sets all base callback functions. */
void vxevent_init_all(VX_NO_ARG);
/* Free memory used by the event queue and hooks. */
void vxevent_destroy(int persistent);

/* Execute all queued events. */
void vxevent_apply_queue(VX_NO_ARG);
/* Add an event to the events queue. */
int vxevent_add(int (*event)(VX_NO_ARG));

/* Add a hook to an event. Use the macro instead. */
void vxevent_hook_add_impl(struct vxstruct_event_hook_base *hook, void (*to_call)(VX_NO_ARG));

VX_C_END

#endif
