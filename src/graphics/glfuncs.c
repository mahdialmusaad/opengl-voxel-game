#include "graphics/glfuncs.h"

#include "directives/dcast.h"

struct vxstruct_gl_funcs gl;

/* Load OpenGL function pointer. */
#define VX_LOAD_GL(name, ret, ...) gl.name = VX_REINT_CAST(ret (VX_APIENTRY *)(__VA_ARGS__), function_loader("gl" #name))

void vxgl_init_ogl(void (*(*function_loader)(const char *function_ascii_name))(VX_NO_ARG))
{
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wcast-function-type-strict"
#endif
	/* Use given function loader on each function pointer member. */
	VX_OPENGL_MASTER(VX_LOAD_GL)
#if defined(__clang__)
# pragma clang diagnostic pop
#endif
}
