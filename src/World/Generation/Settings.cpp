#include "Settings.hpp"

PosType ChunkValues::ToWorld(double x) noexcept { return static_cast<PosType>(glm::floor(x)); }
PosType ChunkValues::ToWorld(float  x) noexcept { return static_cast<PosType>(glm::floor(x)); }

bool ChunkValues::IsOnCorner(const glm::ivec3 &pos, WorldDirection dir) noexcept
{
	const int val = dir < 2 ? pos.x : dir < 4 ? pos.y : pos.z; // Determine if the direction is the X, Y or Z axis
	return val == (!(dir & 1) * ChunkValues::sizeLess); // Odd directions (left, down, back) are always in the negative directions and vice versa
}

void ChunkLookupData::CalculateLookupData() noexcept
{
	// Results for chunk calculation - use to check which block is next to
	// another and in which 'nearby chunk' (if it happens to be outside the current chunk)

	// Create and use given compute shader to calculate results
	ShadersObject::Program compShader("Faces.comp");
	compShader.Use();

	constexpr GLsizeiptr lookupSize = static_cast<GLsizeiptr>(sizeof(std::uint32_t) * static_cast<std::size_t>(ChunkValues::uniqueFaces));

	// Create shader storage buffer for compute shader to output results in
	const GLuint ssbo = OGL::CreateBuffer(GL_SHADER_STORAGE_BUFFER);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, lookupSize, nullptr, GLbitfield{});
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1u, ssbo);

	// Set uniform variables in shader
	const std::int32_t castSize = static_cast<std::int32_t>(ChunkValues::size);
	glUniform1i(compShader.GetLocation("chunkSize"), castSize);
	glUniform3iv(compShader.GetLocation("directions"), 6, &game.constants.worldDirectionsInt[0][0]);

	// Begin shader execution and wait for completion
	constexpr GLuint xGroup = static_cast<GLuint>(Math::roundUpX(ChunkValues::sizeSquared * 6, 32));
	glDispatchCompute(xGroup, 1u, 1u);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // (wait for SSBO)

	// Put SSBO data into lookup data and delete the SSBO
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, GLintptr{}, lookupSize, chunkLookupData);
	glDeleteBuffers(1, &ssbo);

	// (Compute shader is destroyed on scope leave)	
}
