#include "Settings.hpp"

// Define external lookup tables
uint32_t chunk_vals::faces_lookup[chunk_vals::total_faces];

pos_t chunk_vals::to_block_pos_one(double x) noexcept { return static_cast<pos_t>(::floor(x)); }
pos_t chunk_vals::to_block_pos_one(float  x) noexcept { return static_cast<pos_t>(::floor(x)); }

bool chunk_vals::is_bordering(const vector3i &pos, world_dir_en dir) noexcept
{
	const int val = dir < 2 ? pos.x : dir < 4 ? pos.y : pos.z; // Determine if the direction is the X, Y or Z axis
	return val == (!(dir & 1) * chunk_vals::less); // Odd directions (left, down, back) are always in the negative directions and vice versa
}

bool chunk_vals::is_y_outside_bounds(pos_t y) noexcept { return y < 0 || y >= chunk_vals::world_height; }

pos_t chunk_vals::offsets_dist(const world_xzpos *off_a, const world_xzpos *off_b) noexcept
{
	// Returns how many chunks are in between the given chunk offsets
	return math::abs(off_a->x - off_b->x) + math::abs(off_a->y - off_b->y);
}


int chunk_vals::step_dir_to(pos_t start, pos_t end) noexcept { return math::sign(end - start); }
vector2i chunk_vals::step_dir_to(const world_xzpos *start, const world_xzpos *end) noexcept
{
	return { step_dir_to(start->x, end->x), step_dir_to(start->y, end->y) };
}

vector3i chunk_vals::step_dir_to(const world_pos *start, const world_pos *end) noexcept
{
	return { step_dir_to(start->x, end->x), step_dir_to(start->y, end->y), step_dir_to(start->z, end->z) };
}

void chunk_vals::fill_lookup() noexcept
{
	// Results for chunk calculation - use to check which block is next to
	// another and in which 'nearby chunk' (if it happens to be outside the current chunk)

	// Create and use given compute shader to calculate results - destructor called at end
	constexpr GLsizeiptr lookup_bytes = sizeof(uint32_t[chunk_vals::total_faces]);

	// Create shader storage buffer for compute shader to output results in
	const GLuint ssbo = ogl::new_buf(GL_SHADER_STORAGE_BUFFER);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, lookup_bytes, nullptr, GL_CLIENT_STORAGE_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);

	// Set uniform variables in shader
	constexpr size_t dir_size = math::size(chunk_vals::dirs_xyz);
	vector3i32 compute_uniform_dirs[dir_size];
	for (size_t i = 0; i < dir_size; ++i) compute_uniform_dirs[i] = chunk_vals::dirs_xyz[i];

	game.shaders.computes.faces.set_int("cz", chunk_vals::size);
	game.shaders.computes.faces.set_int("qz", chunk_vals::squared);
	glUniform3iv(game.shaders.computes.faces.get_loc("D"), 6, &compute_uniform_dirs[0][0]);
	
	// Begin shader execution and wait for completion
	constexpr GLuint x_group_count = static_cast<GLuint>(math::round_up_to_x(chunk_vals::squared * 6, 32));
	glDispatchCompute(x_group_count, 1, 1);
	
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Wait for completion

	// Put SSBO data into lookup data and delete the SSBO
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, GLintptr{}, lookup_bytes, faces_lookup);
	glDeleteBuffers(1, &ssbo);
}
