#pragma once
#ifndef SOURCE_GRAPHICS_FUNCS_VXL_HDR
#define SOURCE_GRAPHICS_FUNCS_VXL_HDR
/* OpenGL functions (modified from GLAD generator). */

#include "directives/dextern.h"

#include "graphics/gltypes.h" /* IWYU: export */

#if !defined(APIENTRY)
# if defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__)
#  define APIENTRY __stdcall
# else
#  define APIENTRY
# endif
#endif

/* Load all OpenGL functions. */
VX_C_FUNC void vxgl_init_ogl(void(*(*function_loader)(const char *function_ascii_name))(VX_NO_ARG));

/* Macro collection of all OpenGL functions (base name, return type, variadic params). */
#define VX_OPENGL_MASTER(operation)\
operation(ActiveTexture, void, GLenum texture);\
operation(AttachShader, void, GLuint program, GLuint shader);\
operation(BindBuffer, void, GLenum target, GLuint buffer);\
operation(BindBufferBase, void, GLenum target, GLuint index, GLuint buffer);\
operation(BindTexture, void, GLenum target, GLuint texture);\
operation(BindVertexArray, void, GLuint array);\
operation(BlendColor, void, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);\
operation(BlendFunc, void, GLenum sfactor, GLenum dfactor);\
operation(BufferData, void, GLenum target, GLsizeiptr size, const void *data, GLenum usage);\
operation(BufferSubData, void, GLenum target, GLintptr offset, GLsizeiptr size, const void *data);\
operation(ClampColor, void, GLenum target, GLenum clamp);\
operation(Clear, void, GLbitfield mask);\
operation(ClearColor, void, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);\
operation(CompileShader, void, GLuint shader);\
operation(CreateProgram, GLuint, void);\
operation(CreateShader, GLuint, GLenum type);\
operation(CullFace, void, GLenum mode);\
operation(DeleteBuffers, void, GLsizei n, const GLuint *buffers);\
operation(DeleteProgram, void, GLuint program);\
operation(DeleteShader, void, GLuint shader);\
operation(DeleteTextures, void, GLsizei n, const GLuint *textures);\
operation(DeleteVertexArrays, void, GLsizei n, const GLuint *arrays);\
operation(DepthFunc, void, GLenum func);\
operation(DetachShader, void, GLuint program, GLuint shader);\
operation(Disable, void, GLenum cap);\
operation(DrawArrays, void, GLenum mode, GLint first, GLsizei count);\
operation(DrawArraysInstanced, void, GLenum mode, GLint first, GLsizei count, GLsizei instancecount);\
operation(DrawElements, void, GLenum mode, GLsizei count, GLenum type, const void *indices);\
operation(DrawElementsInstanced, void, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);\
operation(Enable, void, GLenum cap);\
operation(EnableVertexAttribArray, void, GLuint index);\
operation(GenBuffers, void, GLsizei n, GLuint *buffers);\
operation(GenTextures, void, GLsizei n, GLuint *textures);\
operation(GenVertexArrays, void, GLsizei n, GLuint *arrays);\
operation(GenerateMipmap, void, GLenum target);\
operation(GetProgramInfoLog, void, GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);\
operation(GetProgramiv, void, GLuint program, GLenum pname, GLint *params);\
operation(GetShaderInfoLog, void, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);\
operation(GetShaderSource, void, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);\
operation(GetShaderiv, void, GLuint shader, GLenum pname, GLint *params);\
operation(GetString, const GLubyte *, GLenum name);\
operation(GetUniformBlockIndex, GLuint, GLuint program, const GLchar *uniformBlockName);\
operation(GetUniformLocation, GLint, GLuint program, const GLchar *name);\
operation(LinkProgram, void, GLuint program);\
operation(MapBuffer, void *, GLenum target, GLenum access);\
operation(PointSize, void, GLfloat size);\
operation(PolygonMode, void, GLenum face, GLenum mode);\
operation(ReadBuffer, void, GLenum src);\
operation(ReadPixels, void, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);\
operation(ShaderSource, void, GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length);\
operation(TexImage2D, void, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);\
operation(TexParameteri, void, GLenum target, GLenum pname, GLint param);\
operation(Uniform1fv, void, GLint location, GLsizei count, const GLfloat *value);\
operation(Uniform1iv, void, GLint location, GLsizei count, const GLint *value);\
operation(Uniform2f, void, GLint location, GLfloat x, GLfloat y);\
operation(Uniform3f, void, GLint location, GLfloat x, GLfloat y, GLfloat z);\
operation(Uniform4f, void, GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);\
operation(UniformBlockBinding, void, GLuint program, GLuint uniformBlockIndex, GLuint uniformblockbinding);\
operation(UnmapBuffer, GLboolean, GLenum target);\
operation(UseProgram, void, GLuint program);\
operation(VertexAttribDivisor, void, GLuint index, GLuint divisor);\
operation(VertexAttribIPointer, void, GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);\
operation(VertexAttribPointer, void, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);\
operation(Viewport, void, GLint x, GLint y, GLsizei width, GLsizei height);\

/* OpenGL function pointers. */
extern struct vxstruct_gl_funcs
{
/* Member definitions for all OpenGL functions. */
#define VX_OPENGL_MEMBER(name, ret, ...) ret (APIENTRY *name)(__VA_ARGS__)
VX_OPENGL_MASTER(VX_OPENGL_MEMBER)
#undef VX_OPENGL_MEMBER
} gl;

#endif
