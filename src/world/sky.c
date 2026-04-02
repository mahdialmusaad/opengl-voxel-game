#include "world/sky.h"

#include "directives/dcast.h"
#include "directives/dfree.h"

#include "graphics/glctx.h"

#include "shaders/ubo.h"

#include "utils/rand.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Initialize clouds data for rendering. */
static int vxsky_gen_clouds(vxrand_state *sky_random)
{
	const float cloud_shape_vertices[24] =  {
		2.5f, 1.0f, 2.5f,  /* Front top right. */
		0.0f, 1.0f, 2.5f,  /* Front top left. */
		2.5f, 0.0f, 2.5f,  /* Front bottom right. */
		0.0f, 0.0f, 2.5f,  /* Front bottom left. */
		2.5f, 1.0f, 0.0f,  /* Back top right. */
		0.0f, 1.0f, 0.0f,  /* Back top left. */
		2.5f, 0.0f, 0.0f,  /* Back bottom right. */
		0.0f, 0.0f, 0.0f,  /* Back bottom left. */
	};
	const unsigned char cloud_vertices_indices[36] = {
		0u, 1u, 2u,  1u, 3u, 2u,  3u, 5u, 7u,  5u, 3u, 1u,
		4u, 6u, 7u,  5u, 4u, 7u,  5u, 1u, 4u,  0u, 4u, 1u,
		4u, 0u, 6u,  6u, 0u, 2u,  2u, 3u, 6u,  3u, 7u, 6u
	};
	
	typedef struct { float x, y, z, size; } vxsky_cloud;
	vxsky_cloud *cloud_data = VX_CAST(vxsky_cloud *, malloc(sizeof *cloud_data * VX_SKY_CLOUDS_COUNT));
	if (!cloud_data) return 0;

	/* Calculate cloud positions and sizes. Data format: { X, Y, Z, size }. */
	for (size_t ind = 0u; ind < VX_SKY_CLOUDS_COUNT; ++ind) {
		const vxsky_cloud cloud = {
			(VX_CAST(float, vxrand_get_double(sky_random)) * VX_SKY_CLOUDS_MAX_DISTANCE) - (VX_SKY_CLOUDS_MAX_DISTANCE * 0.5f),
			(VX_CAST(float, ind) * 0.07f) + VX_SKY_CLOUDS_BASE_Y,
			(VX_CAST(float, vxrand_get_double(sky_random)) * VX_SKY_CLOUDS_MAX_DISTANCE) - (VX_SKY_CLOUDS_MAX_DISTANCE * 0.5f),
			fmaxf(VX_CAST(float, vxrand_get_double(sky_random)), 0.1f) * 30.0f
		};
		memcpy(cloud_data + ind, &cloud, sizeof cloud);
	}

	/* Setup context for clouds rendering. */
	vxctx_init(&vxsd_shaders.clouds,
		VX_CTX_VBO(cloud_shape_vertices),
		VX_CTX_BVBO(cloud_data, sizeof *cloud_data * VX_SKY_CLOUDS_COUNT),
		VX_CTX_EBO(cloud_vertices_indices),
		VX_NULL, vxen_ctxorder_clouds, vxen_ctxmode_tris,
		VX_NULL, 0, VX_NULL
	);

	VX_FREE(cloud_data);
	return 1;
}

/* Initialize skybox data for rendering the sunset and sunrise. */
static void vxsky_gen_skybox(VX_NO_ARG)
{
#if 0
	const float skybox_shape_vertices[] = {
		0.2f, 1.0f, 1.0f,
		0.2f, 1.0f, 0.8f,
		0.4f, 1.0f, 1.0f,
		0.4f, 1.0f, 0.8f
	};

	/* Setup context for skybox rendering. */
	vxctx_init(&vxsd_shaders.sky,
		VX_CTX_VBO(skybox_shape_vertices), VX_CTX_NVBO, VX_CTX_NEBO,
		VX_NULL, vxen_ctxorder_skybox, vxen_ctxmode_tristrips,
		&vxstate_vals.twilight_colour_trnsp, vxen_ctxcond_positivefloat, VX_NULL
	);
#endif
}

/* Initialize stars data for rendering. */
static int vxsky_gen_stars(vxrand_state *sky_random)
{
	/* Using the Fibonnaci sphere algorithm gives evenly distributed positions
	   on a sphere rather than a large proportion being at the top and bottom.
	   Random samples are taken so the stars do not appear in a pattern. */

	typedef struct { uint64_t x : 21, y : 21, z : 22; } vxsky_star;
	vxsky_star *stars_data = VX_CAST(vxsky_star *, malloc(sizeof *stars_data * VX_SKY_STARS_COUNT));
	if (!stars_data) return 0;

	#define VX_FTOI(f) (VX_CAST(uint64_t, (f + 1.0) * VX_CAST(double, (1u << 20u))) & 0x1FFFFFu)

	const double theta_mult = 3.8832220774509331547 * VX_CAST(double, VX_SKY_STARS_COUNT);
	for (size_t i = 0u; i < VX_SKY_STARS_COUNT; ++i) {
		const double index = vxrand_get_double(sky_random);
		const double theta = theta_mult * index, y_pos = 1.0 - (index * 2.0), rad = sqrt(1.0 - y_pos * y_pos);
		const vxsky_star star = { VX_FTOI(cos(theta) * rad), VX_FTOI(y_pos), VX_FTOI(sin(theta) * rad) };
		stars_data[i] = star;
	}

	/* Setup context for stars rendering. */
	vxctx_init(&vxsd_shaders.stars,
		VX_CTX_BVBO(stars_data, sizeof *stars_data * VX_SKY_STARS_COUNT), VX_CTX_NVBO, VX_CTX_NEBO,
		VX_NULL, vxen_ctxorder_stars, vxen_ctxmode_points,
		&vxubo_list.floats.FLT_stars_trnsp, vxen_ctxcond_positivefloat, VX_NULL
	);

	VX_FREE(stars_data);
	return 1;
}

/* Initialize planets (sun and moon) data for rendering. */
static void vxsky_gen_planets(VX_NO_ARG)
{
	/* Relative positions at the start of a day-night cycle. */
	const float large_sun = 0.2f, inner_sun = large_sun * 0.8f, outer_sun = large_sun * 1.4f;
	const float large_moon = 0.05f, inner_moon = large_moon * 0.8f, outer_moon = large_moon * 1.3f;
	const float center_any = 0.0f;

	/* Data format: { X, Y, Z, 1 }, { R, G, B, A }. */
	const float planets_shapes_vertices[][4] = {
		/* Outer sun square. */
		{ -outer_sun,  1.0f,  -outer_sun,  1.0f }, { 0.93f, 0.89f, 0.84f, 0.00f }, /* TL ind 0. */
		{  outer_sun,  1.0f,  -outer_sun,  1.0f }, { 0.93f, 0.89f, 0.84f, 0.00f }, /* BL ind 1. */
		{ center_any,  1.0f,  center_any,  1.0f }, { 0.93f, 0.89f, 0.84f, 1.00f }, /* CN ind 2. */
		{ -outer_sun,  1.0f,   outer_sun,  1.0f }, { 0.93f, 0.89f, 0.84f, 0.00f }, /* TR ind 3. */
		{  outer_sun,  1.0f,   outer_sun,  1.0f }, { 0.93f, 0.89f, 0.84f, 0.00f }, /* BR ind 4. */
		/* Large sun square. */
		{ -large_sun,  1.0f,  -large_sun,  1.0f }, { 0.96f, 0.79f, 0.32f, 1.00f }, /* TL ind 5. */
		{  large_sun,  1.0f,  -large_sun,  1.0f }, { 0.96f, 0.79f, 0.32f, 1.00f }, /* BL ind 6. */
		{ -large_sun,  1.0f,   large_sun,  1.0f }, { 0.96f, 0.79f, 0.32f, 1.00f }, /* TR ind 7. */
		{  large_sun,  1.0f,   large_sun,  1.0f }, { 0.96f, 0.79f, 0.32f, 1.00f }, /* BR ind 8. */
		/* Inner sun square. */
		{ -inner_sun,  1.0f,  -inner_sun,  1.0f }, { 0.92f, 0.85f, 0.74f, 1.00f }, /* TL ind  9. */
		{  inner_sun,  1.0f,  -inner_sun,  1.0f }, { 0.92f, 0.85f, 0.74f, 1.00f }, /* BL ind 10. */
		{ -inner_sun,  1.0f,   inner_sun,  1.0f }, { 0.92f, 0.85f, 0.74f, 1.00f }, /* TR ind 11. */
		{  inner_sun,  1.0f,   inner_sun,  1.0f }, { 0.92f, 0.85f, 0.74f, 1.00f }, /* BR ind 12. */
		/* Right/left order is reversed as the moon is on the opposite side. */
		/* Outer moon square. */
		{ -outer_moon, -1.0f,  outer_moon, 1.0f }, { 0.87f, 0.92f, 0.97f, 0.00f }, /* TR ind 16. */
		{  outer_moon, -1.0f,  outer_moon, 1.0f }, { 0.87f, 0.92f, 0.97f, 0.00f }, /* BR ind 17. */
		{  center_any, -1.0f,  center_any, 1.0f }, { 0.87f, 0.92f, 0.97f, 1.00f }, /* CN ind 15. */
		{ -outer_moon, -1.0f, -outer_moon, 1.0f }, { 0.87f, 0.92f, 0.97f, 0.00f }, /* TL ind 13. */
		{  outer_moon, -1.0f, -outer_moon, 1.0f }, { 0.87f, 0.92f, 0.97f, 0.00f }, /* BL ind 14. */
		/* Large moon square. */
		{ -large_moon, -1.0f,  large_moon, 1.0f }, { 0.56f, 0.68f, 0.95f, 1.00f }, /* TR ind 18. */
		{  large_moon, -1.0f,  large_moon, 1.0f }, { 0.56f, 0.68f, 0.95f, 1.00f }, /* BR ind 19. */
		{ -large_moon, -1.0f, -large_moon, 1.0f }, { 0.56f, 0.68f, 0.95f, 1.00f }, /* TL ind 20. */
		{  large_moon, -1.0f, -large_moon, 1.0f }, { 0.56f, 0.68f, 0.95f, 1.00f }, /* BL ind 21. */
		/* Inner moon square. */
		{ -inner_moon, -1.0f,  inner_moon, 1.0f }, { 0.78f, 0.85f, 0.91f, 1.00f }, /* TR ind 22. */
		{  inner_moon, -1.0f,  inner_moon, 1.0f }, { 0.78f, 0.85f, 0.91f, 1.00f }, /* BR ind 23. */
		{ -inner_moon, -1.0f, -inner_moon, 1.0f }, { 0.78f, 0.85f, 0.91f, 1.00f }, /* TL ind 24. */
		{  inner_moon, -1.0f, -inner_moon, 1.0f }, { 0.78f, 0.85f, 0.91f, 1.00f }, /* BL ind 25. */
	};

	const unsigned char planets_vertices_indices[] = {
		 0,  1,  2,  4,  3,  2, /* Outer sun indices. */
		 3,  0,  2,  1,  4,  2, 
		 5,  6,  8,  8,  7,  5, /* Large sun indices. */
		 9, 10, 12, 12, 11,  9, /* Inner sun indices. */
		13, 14, 15, 17, 16, 15, /* Outer moon indices. */
		16, 13, 15, 14, 17, 15, 
		18, 19, 21, 21, 20, 18, /* Large moon indices. */
		22, 23, 25, 25, 24, 22  /* Inner moon indices. */
	};

	/* Setup context for planets rendering. */
	vxctx_init(&vxsd_shaders.planets,
		VX_CTX_VBO(planets_shapes_vertices), VX_CTX_NVBO, VX_CTX_EBO(planets_vertices_indices),
		VX_NULL, vxen_ctxorder_planets, vxen_ctxmode_tris,
		VX_NULL, 0, VX_NULL
	);
}

int vxsky_init(VX_NO_ARG)
{
	vxrand_state sky_random;
	vxrand_init_state(&sky_random);

	if (!vxsky_gen_clouds(&sky_random)) return 0;
	if (!vxsky_gen_stars(&sky_random)) return 0;

	vxsky_gen_planets();
	vxsky_gen_skybox();

	return 1;
}
