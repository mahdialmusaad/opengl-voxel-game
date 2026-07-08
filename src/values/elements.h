#pragma once
#ifndef SOURCE_VALUES_ELEMENTS_VXL_HDR
#define SOURCE_VALUES_ELEMENTS_VXL_HDR
/* Blocks, structures and biomes. */

#include "directives/dextern.h"

#include "vector/wpos.h"

#include <stddef.h>

/* Width and height of a block texture. */
#define VX_TEX_DIMS (16)

/* Type to store a block ID. */
typedef unsigned short vxblk;
/* Type to store a texture ID. */
typedef unsigned short vxtex;

/* Render type. */
enum
{
	vxen_elmrnd_never,
	vxen_elmrnd_default,
	vxen_elmrnd_transp
};
/* Fixed direction order. */
enum
{
	wdir_right =  0, /* Positive X direction. */
	wdir_top   =  1, /* Positive Y direction. */
	wdir_front =  2, /* Positive Z direction. */
	wdir_left  =  3, /* Negative X direction. */
	wdir_down  =  4, /* Negative Y direction. */
	wdir_back  =  5  /* Negative Z direction. */
};

/* Base direction vectors. */
extern const wpos vxelm_dirs[6];

typedef struct
{
	/* Relative 3D position [0, 1]. */
	float x, y, z;
	/* Relative texture coordinates [0, 1]. */
	float u, v;
} vxelm_mesh_vertex;
typedef struct
{
	char name[104];
	size_t name_size;
	/* Vertex data per direction. */
	vxelm_mesh_vertex *verts[6];
	/* How many vertices there are per direction. */
	int verts_cnts[6];
	/* Index data per direction. */
	unsigned int *ebos[6];
	/* How many indices there are per direction. */
	int ebos_cnts[6];
	/* Whether culling should be disabled for each face. */
	int cull[6];
} vxelm_mesh;
/* List of all loaded mesh data. */
extern vxelm_mesh *vxelm_meshes;
extern size_t vxelm_meshes_count;

/* Element attributes object. */
typedef struct
{
	char name[104];
	vxelm_mesh *mesh;
	short strength;
	unsigned short textures[6];
	unsigned int transparency : 1, solid : 1, face_texs_filled : 3;
	unsigned int name_size : 11;
} vxelm_attribs;
/* List of all loaded element attributes. */
extern vxelm_attribs *vxelm_elements;
extern size_t vxelm_elements_count;

/* Pixel size of a single block texture. */
extern float vxelm_block_texture_y_pixel;

VX_C_START

/* Load game data from files. */
void vxelm_load(VX_NO_ARG);
/* Destroy allocated game data. */
void vxelm_destroy(VX_NO_ARG);

VX_C_END

#endif
