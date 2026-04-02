#include "shaders/ubo.h"

#include "graphics/glfuncs.h"
#include "graphics/glenum.h"

#undef VX_UBO_CREATE

#if defined(__cplusplus)
# define VX_UBO_INITIALIZE_DATA(first, init) {}
#else
# define VX_UBO_INITIALIZE_DATA(first, init) .first = init
#endif

#if defined(__cplusplus)
#define VX_UBO_STRUCTNAME(name) vxstruct_ubo_list::vxstruct_ubo_##name##_object
#else
#define VX_UBO_STRUCTNAME(name) struct vxstruct_ubo_##name##_object
#endif

/* Initialize UBO from given parameters. */
#define VX_UBO_CREATE(name, glsl_type_name, type, init, first, ...)\
{\
	{\
		"layout(std140)uniform UBO_" #name "{" #glsl_type_name " " #first "," #__VA_ARGS__ ";};",\
		"UBO_" #name, 0u, sizeof(VX_UBO_STRUCTNAME(name)) - offsetof(VX_UBO_STRUCTNAME(name), first)\
	},\
	offsetof(VX_UBO_STRUCTNAME(name), first),\
	VX_UBO_INITIALIZE_DATA(first, init)\
},

#define VX_UBO_ADD_BASE(name, ...) &vxubo_list.name.base,

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

struct vxstruct_ubo_list vxubo_list = { VX_UBO_MASTER(VX_UBO_CREATE) };
struct vxstruct_ubo_base_instance *vxubo_bases[sizeof vxubo_bases / sizeof *vxubo_bases] = { VX_UBO_MASTER(VX_UBO_ADD_BASE) };

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

void vxubo_update_direct(struct vxstruct_ubo_base_instance *ubo_inst, const void *data, size_t offset, size_t bytes)
{
	gl.BindBuffer(GL_UNIFORM_BUFFER, ubo_inst->id);
	gl.BufferSubData(GL_UNIFORM_BUFFER, VX_CAST(GLintptr, offset), VX_CAST(GLsizeiptr, bytes), data);
}
