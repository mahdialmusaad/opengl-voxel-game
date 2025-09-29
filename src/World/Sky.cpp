#include "Sky.hpp"

skybox_elems_obj::skybox_elems_obj() noexcept
{
	// Random float generator in range [0.0f, 1.0f)
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	generate_clouds(&gen, &dist);
	generate_stars((dist(gen) * 0.2f) + 0.4f);
	generate_skybox();
	generate_planets();
}

void skybox_elems_obj::draw_skybox_elements() noexcept
{
	// Transition skybox (orange section at sunrise/set)
	/* TODO: Sky shape
	if (game.twilight_colour_trnsp > 0.0f) {
		game.shaders.programs.sky.bind_and_use(m_skybox_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 0);
	}*/

	// Stars
	if (game.shaders.times.vals.stars > 0.0f) {
		game.shaders.programs.stars.bind_and_use(m_stars_vao);
		glDrawArrays(GL_POINTS, 0, stars_count);
	}

	// Planets (sun and moon)
	game.shaders.programs.planets.bind_and_use(m_planets_vao);
	glDrawElements(GL_TRIANGLES, 48, GL_UNSIGNED_BYTE, nullptr);
	
	// Clouds
	game.shaders.programs.clouds.bind_and_use(m_clouds_vao);
	glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, nullptr, clouds_count);
}

void skybox_elems_obj::generate_clouds(std::mt19937 *gen, std::uniform_real_distribution<float> *dist) noexcept
{
	m_clouds_vao = ogl::new_vao();
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1); // Instanced rendering

	// Cloud shape data
	const float cloud_verts_def[24] =  {
		2.5f, 1.0f, 2.5f,  // Front-top-right     0
		0.0f, 1.0f, 2.5f,  // Front-top-left      1
		2.5f, 0.0f, 2.5f,  // Front-bottom-right  2
		0.0f, 0.0f, 2.5f,  // Front-bottom-left   3
		2.5f, 1.0f, 0.0f,  // Back-top-right      4
		0.0f, 1.0f, 0.0f,  // Back-top-left       5
		2.5f, 0.0f, 0.0f,  // Back-bottom-right   6
		0.0f, 0.0f, 0.0f,  // Back-bottom-left    7
	};

	// Buffer for cloud vertices
	m_clouds_vbo = ogl::new_buf(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof cloud_verts_def, cloud_verts_def, 0);
	
	// Buffer for indexes into cloud vertices
	const GLubyte cloud_inds[36] = {
		0, 1, 2,  1, 3, 2, 
		3, 5, 7,  5, 3, 1, 
		4, 6, 7,  5, 4, 7, 
		5, 1, 4,  0, 4, 1, 
		4, 0, 6,  6, 0, 2, 
		2, 3, 6,  3, 7, 6
	};
	m_clouds_ebo = ogl::new_buf(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof cloud_inds, cloud_inds, 0);
	
	// Setup instanced cloud buffer
	m_clouds_inst_vbo = ogl::new_buf(GL_ARRAY_BUFFER);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Cloud generation
	vector4f *const cloud_data = new vector4f[clouds_count]; // Cloud data
	constexpr float xz_mult = 2000.0f, xz_half = xz_mult * 0.5f, sz_mult = 30.0f;

	for (int ind = 0; ind < clouds_count; ++ind) {
		cloud_data[ind] = {
			((*dist)(*gen) * xz_mult) - xz_half,       // X position
			198.7f + (static_cast<float>(ind) * 0.07f),  // Y position
			((*dist)(*gen) * xz_mult) - xz_half,       // Z position
			math::max((*dist)(*gen), 0.02f) * sz_mult     // Size
		};
	}

	// Buffer the float data into the GPU (in instanced buffer)
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(vector4f[clouds_count]), cloud_data, 0);

	// Free memory used by cloud data
	delete[] cloud_data;
}

void skybox_elems_obj::generate_skybox() noexcept
{
	// Create skybox buffers
	m_skybox_vao = ogl::new_vao();
	glEnableVertexAttribArray(0);

	/* 
	   Since the sky is just a single colour (from glClearColor), a specific 
	   part of it can have triangles covering that changes colour and fade 
	   to the 'cleared' sky colour to imitate a sunrise/set. This saves on 
	   fragment shader calculations for the rest of the sky.
	*/

	// Skybox vertex data
	const float skybox_vertex_def[] = {
		0.2f, 0.0f, 1.0f,
		0.2f, 0.2f, 0.8f,
		0.4f, 0.2f, 1.0f,
		0.4f, 0.2f, 0.8f
	};

	// Setup sky vertices buffer
	m_skybox_vbo = ogl::new_buf(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof skybox_vertex_def, skybox_vertex_def, 0);
}

void skybox_elems_obj::generate_stars(float init_random) noexcept
{
	/* 
	   Using the Fibonnaci sphere algorithm gives evenly distributed positions
	   on a sphere rather than a large proportion being at the top and bottom. 
	   Random samples are taken so the stars do not appear in a pattern.
	   
	   phi = pi * (sqrt(5) - 1) = 3.883222077...
	   i = [0, max)

	   theta = phi * i
	   y = 1 - (i / max) * 2
	   radius = sqrt(1 - y * y)
	   
	   x = cos(theta) * radius
	   z = sin(theta) * radius

	   This algorithm is executed in a compute shader, where the GPU will
	   create large numbers of stars in parallel.
	*/

	// Create SSBO for star data results
	const GLuint ssbo = ogl::new_buf(GL_SHADER_STORAGE_BUFFER);
	const GLsizeiptr stars_bytes = sizeof(vector3f[stars_count]);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, stars_bytes, nullptr, GL_CLIENT_STORAGE_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);

	// Set uniform variables required in compute shader
	game.shaders.computes.stars.set_uint("SC", static_cast<GLuint>(stars_count));
	game.shaders.computes.stars.set_flt("I", init_random);
	
	glDispatchCompute(static_cast<GLuint>(math::round_up_to_x(stars_count / 32, 32)), 1, 1); // Execute compute shader
	// Whilst it is executing...

	m_stars_vao = ogl::new_vao(); // Create stars VAO
	glEnableVertexAttribArray(0); // Attribute 0 for vec3

	m_stars_vbo = ogl::new_buf(GL_ARRAY_BUFFER); // Create array buffer for vec3 data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); // Vec3 on attribute 0
	glBufferStorage(GL_ARRAY_BUFFER, stars_bytes, nullptr, GL_CLIENT_STORAGE_BIT); // Buffer calculated data
	
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Wait for compute shader to finish all execution
	glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_ARRAY_BUFFER, 0, 0, stars_bytes); // Copy SSBO data to buffer data
	glDeleteBuffers(1, &ssbo); // Delete created SSBO
}

void skybox_elems_obj::generate_planets() noexcept
{
	// Planets buffers
	m_planets_vao = ogl::new_vao();
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	// Place sun and moon directly opposite each other with same positioning as stars as seen above
	// Both the sun and moon (for now) are three squares: a larger, dark one, a smaller, lighter one on top,
	// and an 'outer' even larger matching square to imitate intense light from the sun/moon that transitions into the bg.

	// Positions at 0 seconds of in-game day time
	const float large_sun = 0.2f, inner_sun = large_sun * 0.8f, outer_sun = large_sun * 1.4f;
	const float large_moon = 0.05f, inner_moon = large_moon * 0.8f, outer_moon = large_moon * 1.3f;
	
	// Planets use a 'zero' matrix, where only the camera's rotation is taken into
	// account, so they always appear around the camera without having to change position.
	// TODO: instancing? somehow?
	const vector4f planets_verts_def[] = {
		//     X        Y         Z       1.0f        R      G      B      A
		// Outer sun square
		{ -outer_sun,   1.0f, -outer_sun,  1.0f }, { 0.93f, 0.89f, 0.84f, 0.00f }, // TL 0
		{  outer_sun,   1.0f, -outer_sun,  1.0f }, { 0.93f, 0.89f, 0.84f, 0.00f }, // BL 1
		{  0.0f,        1.0f,  0.0f,       1.0f }, { 0.93f, 0.89f, 0.84f, 1.00f }, // CN 2 - center used for vertex colour blending
		{ -outer_sun,   1.0f,  outer_sun,  1.0f }, { 0.93f, 0.89f, 0.84f, 0.00f }, // TR 3
		{  outer_sun,   1.0f,  outer_sun,  1.0f }, { 0.93f, 0.89f, 0.84f, 0.00f }, // BR 4
		// Large sun square
		{ -large_sun,   1.0f, -large_sun,  1.0f }, { 0.96f, 0.79f, 0.32f, 1.00f }, // TL 5
		{  large_sun,   1.0f, -large_sun,  1.0f }, { 0.96f, 0.79f, 0.32f, 1.00f }, // BL 6
		{ -large_sun,   1.0f,  large_sun,  1.0f }, { 0.96f, 0.79f, 0.32f, 1.00f }, // TR 7
		{  large_sun,   1.0f,  large_sun,  1.0f }, { 0.96f, 0.79f, 0.32f, 1.00f }, // BR 8
		// Inner sun square
		{ -inner_sun,   1.0f, -inner_sun,  1.0f }, { 0.92f, 0.85f, 0.74f, 1.00f }, // TL 9
		{  inner_sun,   1.0f, -inner_sun,  1.0f }, { 0.92f, 0.85f, 0.74f, 1.00f }, // BL 10
		{ -inner_sun,   1.0f,  inner_sun,  1.0f }, { 0.92f, 0.85f, 0.74f, 1.00f }, // TR 11
		{  inner_sun,   1.0f,  inner_sun,  1.0f }, { 0.92f, 0.85f, 0.74f, 1.00f }, // BR 12
		// Outer moon square - square order is reversed as it is on the opposite side
		{ -outer_moon, -1.0f,  outer_moon, 1.0f }, { 0.87f, 0.92f, 0.97f, 0.00f }, // TR 16
		{  outer_moon, -1.0f,  outer_moon, 1.0f }, { 0.87f, 0.92f, 0.97f, 0.00f }, // BR 17
		{  0.0f,       -1.0f,  0.0f,       1.0f }, { 0.87f, 0.92f, 0.97f, 1.00f }, // CN 15
		{ -outer_moon, -1.0f, -outer_moon, 1.0f }, { 0.87f, 0.92f, 0.97f, 0.00f }, // TL 13
		{  outer_moon, -1.0f, -outer_moon, 1.0f }, { 0.87f, 0.92f, 0.97f, 0.00f }, // BL 14
		// Large moon square
		{ -large_moon, -1.0f,  large_moon, 1.0f }, { 0.56f, 0.68f, 0.95f, 1.00f }, // TR 18
		{  large_moon, -1.0f,  large_moon, 1.0f }, { 0.56f, 0.68f, 0.95f, 1.00f }, // BR 19
		{ -large_moon, -1.0f, -large_moon, 1.0f }, { 0.56f, 0.68f, 0.95f, 1.00f }, // TL 20
		{  large_moon, -1.0f, -large_moon, 1.0f }, { 0.56f, 0.68f, 0.95f, 1.00f }, // BL 21
		// Inner moon square
		{ -inner_moon, -1.0f,  inner_moon, 1.0f }, { 0.78f, 0.85f, 0.91f, 1.00f }, // TR 22
		{  inner_moon, -1.0f,  inner_moon, 1.0f }, { 0.78f, 0.85f, 0.91f, 1.00f }, // BR 23
		{ -inner_moon, -1.0f, -inner_moon, 1.0f }, { 0.78f, 0.85f, 0.91f, 1.00f }, // TL 24
		{  inner_moon, -1.0f, -inner_moon, 1.0f }, { 0.78f, 0.85f, 0.91f, 1.00f }, // BL 25
	};

	// Create VBO and buffer above vertex data
	m_planets_vbo = ogl::new_buf(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0, 4, GL_FLOAT, false, sizeof(float[8]), nullptr); // Using mat2x4 - 8 floats, 4 floats per attribute
	glVertexAttribPointer(1, 4, GL_FLOAT, false, sizeof(float[8]), reinterpret_cast<const void*>(sizeof(float[4])));
	glBufferStorage(GL_ARRAY_BUFFER, sizeof planets_verts_def, planets_verts_def, 0);

	// An EBO (indexes into above vertices) needs to be used with GL_TRIANGLES as the sun and
	// moon are separate objects and using GL_TRIANGLE_STRIPS would join them together.
	// Using plain glDrawArrays means some vertices will need to be redefined - save memory
	// by using glDrawElements alongside an index list.

	// Every trio of u. bytes is a triangle 
	const GLubyte planets_inds[] = {
		/* Outer sun indexes  */ 0,  1,  2,  4,  3,  2, 
		                         3,  0,  2,  1,  4,  2,
		/* Large sun indexes  */ 5,  6,  8,  8,  7,  5,
		/* Inner sun indexes  */ 9,  10, 12, 12, 11, 9,
	
		/* Outer moon indexes */ 13, 14, 15, 17, 16, 15,
		                         16, 13, 15, 14, 17, 15,
		/* Large moon indexes */ 18, 19, 21, 21, 20, 18,
		/* Inner moon indexes */ 22, 23, 25, 25, 24, 22
	};

	// Create EBO and buffer above indices - use GLubyte in glDrawElements for correct data interpreting
	m_planets_ebo = ogl::new_buf(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof planets_inds, planets_inds, 0);
}

skybox_elems_obj::~skybox_elems_obj()
{
	// Delete all created buffers (VBOs, EBOs)
	const GLuint delete_bufs[] = {
		m_clouds_inst_vbo,
		m_clouds_vbo,
		m_clouds_ebo,
		m_skybox_vbo,
		m_stars_vbo,
		m_planets_vbo,
		m_planets_ebo
	};

	glDeleteBuffers(static_cast<GLsizei>(math::size(delete_bufs)), delete_bufs);

	// Delete all created VAOs
	const GLuint delete_vaos[] = {
		m_clouds_vao,
		m_skybox_vao,
		m_stars_vao,
		m_planets_vao
	};

	glDeleteVertexArrays(static_cast<GLsizei>(math::size(delete_vaos)), delete_vaos);
}
