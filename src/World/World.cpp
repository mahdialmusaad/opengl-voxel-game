#include "World.hpp"

World::World(PlayerObject& player) noexcept : player(player)
{
	TextFormat::log("World enter");

	// Create VAO for the world buffers to store buffer settings
	m_worldVAO = OGL::CreateVAO8();
	glEnableVertexAttribArray(0u);
	glVertexAttribDivisor(0u, 1u);

	glEnableVertexAttribArray(1u);
	glEnableVertexAttribArray(2u);
	glEnableVertexAttribArray(3u);

	// World data buffer for instanced compressed face data
	m_worldInstancedVBO = OGL::CreateBuffer8(GL_ARRAY_BUFFER);
	glVertexAttribIPointer(0u, 1, GL_UNSIGNED_INT, 0, nullptr);

	// Very important definition of a plane/quad for block rendering
	// Laid out like this to allow for swizzling (e.g. verticesXZ.xyz) which is 'essentially free' in shaders
	// See https://www.khronos.org/opengl/wiki/GLSL_Optimizations for more information
	constexpr float planeVertices[] = {
	//	        vec4 0		                 vec4 1		                vec4 2
		1.0f, 1.0f, 0.0f, 1.0f, 	0.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f, 0.0f, 0.0f,	 // Vertex 1
		1.0f, 0.0f, 0.0f, 1.0f, 	0.0f, 0.0f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f, 0.0f,	 // Vertex 2
		0.0f, 1.0f, 0.0f, 1.0f, 	1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f, 0.0f, 0.0f,	 // Vertex 3
		0.0f, 0.0f, 0.0f, 1.0f, 	1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 1.0f, 0.0f, 0.0f,	 // Vertex 4
	//	 x     z     0     1         y     z     0     1         y     w     0     0
	};

	// Instanced VBO that contains plane data, 12 floats per vertex
	m_worldPlaneVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(1u, 4, GL_FLOAT, GL_FALSE, sizeof(float[12]), nullptr);
	glVertexAttribPointer(2u, 4, GL_FLOAT, GL_FALSE, sizeof(float[12]), reinterpret_cast<const void*>(sizeof(float[4])));
	glVertexAttribPointer(3u, 4, GL_FLOAT, GL_FALSE, sizeof(float[12]), reinterpret_cast<const void*>(sizeof(float[8])));
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, 0u);

	// Shader storage buffer object (SSBO) to store chunk positions and face indexes for each chunk face (location = 0)
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0u, OGL::CreateBuffer(GL_SHADER_STORAGE_BUFFER));
	// Buffer which holds the indexes into the world data for each 'instanced draw call' in indirect draw call
	OGL::CreateBuffer(GL_DRAW_INDIRECT_BUFFER);

	// Possible: noise splines for varied terrain generation
	game.noiseGenerators = WorldNoise(nullptr);

	// Initial update and buffer sizing
	UpdateRenderDistance(chunkRenderDistance);

	TextFormat::log("World exit");
}

void World::DrawWorld() const noexcept
{
	// Use correct VAO and shader program
	game.shader.UseShader(Shader::ShaderID::Blocks);
	glBindVertexArray(m_worldVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_worldInstancedVBO);

	// Draw the entire world in a single draw call using the indirect buffer, essentially
	// doing an instanced draw call (glDrawArraysInstancedBaseInstance) for each 'chunk face'.
	// See https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMultiDrawArraysIndirect.xhtml for more information.

	glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, nullptr, m_indirectCalls, 0);
}

Chunk* World::WorldPositionToChunk(PosType x, PosType y, PosType z) const noexcept
{
	// Gets the chunk that contains position x, y, z
	return GetChunk(ChunkSettings::WorldPositionToOffset(x, y, z));
}

ObjectID World::GetBlock(PosType x, PosType y, PosType z) const noexcept
{
	// Get the chunk from the given position
	const Chunk* chunk = WorldPositionToChunk(x, y, z);

	// If the chunk exists, return the block found at the position in local chunk coordinates
	if (chunk != nullptr) {
		return chunk->chunkBlocks->GetBlock(
			ChunkSettings::WorldToLocalPosition(x),
			ChunkSettings::WorldToLocalPosition(y),
			ChunkSettings::WorldToLocalPosition(z)
		);
	}

	// If no chunk is found in the given position, return air
	return ObjectID::Air;
}

ObjectID* World::EditBlock(PosType x, PosType y, PosType z) const noexcept
{
	// Get the chunk from the given position
	const Chunk *chunk = WorldPositionToChunk(x, y, z);

	// If the chunk exists, return a pointer block found at the position in local chunk coordinates
	if (chunk != nullptr) {
		ConvertChunkStorageType(chunk->chunkBlocks); // Convert chunk if it is non-editable
		// Get the chunk array index from the block's position
		const glm::ivec3 offsetPosition = ChunkSettings::WorldToLocalPosition(x, y, z);
		const int blockIndex = ChunkSettings::IndexFromLocalPosition(offsetPosition.x, offsetPosition.y, offsetPosition.z);
		// Return a pointer to the block
		return &dynamic_cast<ChunkSettings::ChunkBlockValueFull*>(chunk->chunkBlocks)->chunkBlocks[blockIndex];
	}

	// If no chunk is found in the given position, do not return a block
	return nullptr;
}

ModifyWorldResult World::SetBlock(PosType x, PosType y, PosType z, ObjectID objectID) noexcept
{
	// Warning text if the set position is above or below world height
	if (y > ChunkSettings::MAX_WORLD_HEIGHT) {
		textRenderer.CreateText({ 0.4f, 0.6f }, "Cannot change blocks above world height", TextRenderer::T_Type::Temporary);
		return ModifyWorldResult::AboveWorld;
	}
	else if (y < 0) {
		textRenderer.CreateText({ 0.4f, 0.6f }, "Cannot change blocks below y = 0", TextRenderer::T_Type::Temporary);
		return ModifyWorldResult::BelowWorld;
	}

	Chunk* chunk = WorldPositionToChunk(x, y, z);

	if (chunk != nullptr) {
		ConvertChunkStorageType(chunk->chunkBlocks); // Possibly convert to standard array chunk
		const glm::ivec3 bps = ChunkSettings::WorldToLocalPosition(x, y, z); // Get the chunk position from the world position
		chunk->chunkBlocks->SetBlock(bps.x, bps.y, bps.z, objectID); // Set the block at said chunk position
		CalculateChunk(chunk); // Calculate the chunk vertices
		UpdateWorldBuffers(); // Update world buffers

		return ModifyWorldResult::Passed;
	}

	return ModifyWorldResult::NotFound;
}

void World::ConvertChunkStorageType(ChunkSettings::ChunkBlockValue *blockArray) const noexcept
{
	// Convert to full storage if it is a single block chunk array
	if (blockArray->GetChunkBlockType() == ChunkSettings::ChunkBlockValueType::Air) {
		delete blockArray;
		blockArray = new ChunkSettings::ChunkBlockValueFull;
	}
}

Chunk* World::SetBlockSimple(PosType x, PosType y, PosType z, ObjectID objectID) const noexcept
{
	// Simpler set block function with less features for quick changes
	Chunk* chunk = WorldPositionToChunk(x, y, z);

	if (chunk != nullptr) {
		ConvertChunkStorageType(chunk->chunkBlocks); // Possibly convert chunk block storage type
		const glm::ivec3 bps = ChunkSettings::WorldToLocalPosition(x, y, z); // Get the chunk position from the world position
		chunk->chunkBlocks->SetBlock(bps.x, bps.y, bps.z, objectID); // Set the block at said chunk position
	}

	// Return the edited chunk
	return chunk;
}

void World::FillBlocks(
	PosType x, PosType y, PosType z,
	PosType tx, PosType ty, PosType tz,
	ObjectID objectID
) noexcept
{
	// Force valid position
	constexpr PosType zero_pt = static_cast<PosType>(0);
	y = Math::clamp(y, zero_pt, ChunkSettings::PMAX_WORLD_HEIGHT);
	ty = Math::clamp(ty, zero_pt, ChunkSettings::PMAX_WORLD_HEIGHT);

	// List of unique edited chunks for updating
	std::unordered_map<WorldPos, Chunk*, WorldPosHash> uniqueChunks;

	// Set all of the valid blocks from x, y, z to tx, ty, tz (inclusive) as the given 'block ID'
	for (PosType _x = x; _x <= tx; ++_x) {
		for (PosType _y = y; _y <= ty; ++_y) {
			for (PosType _z = z; _z <= tz; ++_z) {
				Chunk* changedChunk = SetBlockSimple(_x, _y, _z, objectID);
				uniqueChunks.insert({ changedChunk->GetOffset(), changedChunk });
			}
		}
	}

	// Update all affected chunks and world buffers
	for (const auto& [offset, chunk] : uniqueChunks) CalculateChunk(chunk);
	UpdateWorldBuffers();
}

Chunk* World::GetChunk(WorldPos chunkOffset) const noexcept
{
	const auto it = chunks.find(chunkOffset); // Find chunk with given offset key
	return it != chunks.end() ? it->second : nullptr; // Return chunk if it was found
}

bool World::ChunkExists(WorldPos chunkOffset) const noexcept
{
	// Return true if a chunk at the given offset exists
	return GetChunk(chunkOffset) != nullptr;
}

bool World::InRenderDistance(WorldPos &playerOffset, const WorldPos &chunkOffset) noexcept
{
	return Math::qabs(playerOffset.x - chunkOffset.x) +
		   Math::qabs(playerOffset.z - chunkOffset.z) <= static_cast<PosType>(chunkRenderDistance);
}

void World::UpdateRenderDistance(std::int32_t newRenderDistance) noexcept
{
	// Render distance determines how many chunks in a 'star' pattern will be generated
	// from the initial player chunk (e.g. a render distance of 0 has only the player's chunk,
	// whereas a render distance of 1 has 4 extra chunks surrounding it - 5 in total).

	// Negative render distance (achieved by decreasing from 0 render distance) does not make sense
	if (newRenderDistance < 0) return;
	chunkRenderDistance = newRenderDistance;
	
	// Maximum amount of chunk faces (calculated as (2n^2 + 2n + 1) * h, 
	// where n is the render distance and h is the number of chunks in a'full chunk')
	// Also multiply by 2 due to translucent faces needing to be rendered seperately
	const std::size_t maxChunkFaces = (
			(2 * chunkRenderDistance * chunkRenderDistance) + (2 * chunkRenderDistance) + 1
		) * ChunkSettings::HEIGHT_COUNT * 2;

	// Ensure correct buffers are used
	glBindVertexArray(m_worldVAO);

	// Update buffer sizes (used to avoid doing glBufferData each time, which reallocates = expensive):

	// Indirect buffer ('draw commands' used for rendering, see 'draw world' function)
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectDrawCommand) * maxChunkFaces, nullptr, GL_STATIC_DRAW);
	// SSBO (used to store offset data and face index in world shader)
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ShaderChunkOffset) * maxChunkFaces, nullptr, GL_STATIC_DRAW);

	// Fill the newly sized buffers (empty now) with new valid data
	STChunkUpdate();
	SortWorldBuffers();
}

void World::RemoveChunk(Chunk* chunk) noexcept
{
	// Remove from map and free up memory using delete as it was created with 'new'
	chunks.erase({ chunk->GetOffset() });
	delete chunk;
}

void World::CreateFullChunk(ChunkOffset offsetXZ) noexcept
{
	// Precalculate the noise values for terrain generation
	WorldPerlin::NoiseResult* noiseResults = new WorldPerlin::NoiseResult[ChunkSettings::CHUNK_SIZE_SQUARED];
	SetPerlinValues(noiseResults, offsetXZ * ChunkSettings::PCHUNK_SIZE);

	// Create a new chunk at each Y offset and add it to the chunks map
	WorldPos offset = { offsetXZ.x, 0, offsetXZ.y };
	for (; offset.y < ChunkSettings::PHEIGHT_COUNT; ++offset.y) {
		Chunk* newChunk = new Chunk(offset, noiseResults);
		chunks.insert({ offset, newChunk });
		m_transferChunks.emplace_back(newChunk);
	}

	// Clear up data from noise results
	delete[] noiseResults;
}

void World::CalculateChunk(Chunk* chunk) const noexcept
{
	// Calculate the given chunk, giving the list of chunks as argument (to find nearby chunks)
	const Chunk::ChunkGetter getter = [&](const WorldPos& pos) -> Chunk* {
		const auto& foundChunk = chunks.find(pos);
		return foundChunk == chunks.end() ? nullptr : foundChunk->second;
	};

	chunk->CalculateChunk(getter);
}

void World::SetPerlinValues(WorldPerlin::NoiseResult* results, ChunkOffset offset) noexcept
{
	// (used per full chunk, each chunk would have the same results as they
	// have the same XZ coordinates so no calculation is needed for each individual chunk)

	// Loop through X and Z axis
	for (int i = 0; i < ChunkSettings::CHUNK_SIZE_SQUARED; ++i) {
		const int indZ = i / ChunkSettings::CHUNK_SIZE, indX = i % ChunkSettings::CHUNK_SIZE; // Get the X and Z position
		// Get noise coordinates
		const double posX = static_cast<double>(offset.x + indX) * ChunkSettings::NOISE_STEP,
			posZ = static_cast<double>(offset.y + indZ) * ChunkSettings::NOISE_STEP;

		// Store the height result for each of the terrain noise generators
		WorldPerlin::NoiseResult& result = results[i];
		constexpr int octaves = 3;

		result.continentalnessHeight =
			((game.noiseGenerators.continentalness.Octave2D(posX, posZ, octaves) + 0.4f) *
			ChunkSettings::CHUNK_NOISE_MULTIPLIER) + 20;
		result.flatness = game.noiseGenerators.temperature.Noise2D(posX, posZ) + 0.2f;
		result.temperature = game.noiseGenerators.flatness.Noise2D(posX, posZ) + 0.2f;
		result.humidity = game.noiseGenerators.humidity.Noise2D(posX, posZ) + 0.2f;
	}
}

void World::StartThreadChunkUpdate() noexcept
{
	// TODO
}

void World::STChunkUpdate() noexcept
{
	const Chunk::ChunkGetter finder = [&](const WorldPos& o) noexcept {
		const auto& a = chunks.find(o);
		return a == chunks.end() ? nullptr : a->second;
	};

	std::vector<Chunk*> toRemove;
	for (const auto& [offset, chunk] : chunks) {
		if (!InRenderDistance(player.offset, offset)) {
			toRemove.emplace_back(chunk);
		}
	}

	for (Chunk* p : toRemove) RemoveChunk(p);
	toRemove.clear();

	const ChunkOffset playerOffset = { player.offset.x, player.offset.z };

	// Find valid coordinates relative to the player
	for (int x = -chunkRenderDistance; x <= chunkRenderDistance; ++x) {
		for (int z = -chunkRenderDistance; z <= chunkRenderDistance; ++z) {
			if (Math::qabs(x) + Math::qabs(z) > chunkRenderDistance) continue;

			const ChunkOffset newOffset = playerOffset + ChunkOffset(x, z);
			// Check if there isn't already a chunk at the selected offset
			if (chunks.find({ newOffset.x, 0, newOffset.y }) != chunks.end()) continue;
			CreateFullChunk(newOffset);
		}
	}

	std::unordered_map<WorldPos, Chunk*, WorldPosHash> unique;

	for (Chunk* newChunk : m_transferChunks) {
		unique.insert({ newChunk->GetOffset(), newChunk });
		const WorldPos c = newChunk->GetOffset();
		for (const WorldPos& p : ChunkSettings::worldDirectionsXZ) {
			Chunk* n = finder(c + p);
			if (n != nullptr) unique.insert({ n->GetOffset(), n });
		}
	}

	for (const auto& [offset, chunk] : unique) chunk->CalculateChunk(finder);

	m_transferChunks.clear();
	UpdateWorldBuffers();
}

void World::GenerateTree(TreeGenerateVars vars) noexcept
{
	bool shouldSpawn = static_cast<int>(vars.noiseResult.humidity * 3279245.8f) % 13 == 0;
	if (!shouldSpawn) return;

	enum PlacementResult { Success, Overflow, Blocked };

	const auto TrySetLeaves = [&](PosType lx, PosType ly, PosType lz) noexcept {
		ObjectID *currentBlock = EditBlock(lx, ly, lz);
		if (currentBlock == nullptr) {
			m_blockQueue.at(ChunkSettings::WorldPositionToOffset(lx, ly, lz)).emplace_back(BlockQueue({ lx, ly, lz }, vars.leaves));
			return Overflow;
		}
		else if (ChunkSettings::GetBlockData(*currentBlock).natureReplaceable) {
			*currentBlock = vars.leaves;
			return Success;
		}
		else return Blocked;
	};

	const auto TrySetLog = [&](PosType ty) noexcept {
		ObjectID *currentBlock = EditBlock(vars.x, ty, vars.z);
		if (currentBlock == nullptr) {
			m_blockQueue.at(ChunkSettings::WorldPositionToOffset(vars.x, ty, vars.z)).emplace_back(BlockQueue({ vars.x, ty, vars.z }, vars.log));
			return Overflow;
		}
		else if (ChunkSettings::GetBlockData(*currentBlock).natureReplaceable) {
			*currentBlock = vars.leaves;
			return Success;
		}
		else return Blocked;
	};

	const int height = static_cast<int>(abs(vars.noiseResult.flatness) * 15.0f);
	if (height <= 4) return;

	for (PosType logY = vars.y, logEndY = logY + height; logY < logEndY; ++logY) if (TrySetLog(logY) == Blocked) break;

	/*enum LeavesShape : int { Square, Circular };
	LeavesShape shape = height >= 6 ? Square : Circular;*/

	for (PosType leavesY = vars.y + 3, leavesEndY = leavesY + height - 1; leavesY < leavesEndY; ++leavesY) {
		for (PosType leavesX = vars.x - 2, leavesEndX = leavesX + 4; leavesX <= leavesEndX; ++leavesX) {
			for (PosType leavesZ = vars.z - 2, leavesEndZ = leavesZ + 4; leavesZ <= leavesEndZ; ++leavesZ) {
				TrySetLeaves(leavesX, leavesY, leavesZ);
			}
		}
	}
}

void World::UpdateWorldBuffers() noexcept
{
	//TextFormat::log("World buffer update");

	if (threadUpdateBuffers) {
		// Move any newly generated chunks (from thread) to the main chunk map
		for (Chunk* chunk : m_transferChunks) chunks.insert({ chunk->GetOffset(), chunk });
		m_transferChunks.clear(); // Clear new chunk pointer array
	}

	// Exit if there are no chunks (e.g. moving very fast so all chunks are deleted)
	if (!chunks.size()) {
		SortWorldBuffers();
		threadUpdateBuffers = false;
		return;
	}

	#ifndef BADCRAFT_1THREAD
	// Vectors of chunks to be removed - cannot delete while looping through map at the same time
	std::vector<Chunk*> chunksToRemove;
	#endif

	// Counters
	std::size_t faceDataPointersCount = 0u;
	squaresCount = 0u;

	// The total number of 'chunk faces' (6 per chunk)
	const std::size_t chunkFacesTotal = chunks.size() * static_cast<std::size_t>(6);

	// Pointers to chunk face objects
	Chunk::FaceAxisData** faceDataPointers = new Chunk::FaceAxisData*[chunkFacesTotal];

	// Loop through all of the chunks and each of their 6 face data to determine how much memory is needed
	// overall and accumulate all the valid pointer data into the array, as well as preparing to delete any far away chunks
	for (const auto& [offset, chunk] : chunks) {
		#ifndef BADCRAFT_1THREAD
		// Add to deletion vector if the amount of offsets between the two is larger than current render distance
		if (!ChunkSettings::InRenderDistance(player.offset, offset)) {
			chunksToRemove.emplace_back(chunk);
			continue;
		}
		#endif

		// Loop through all of the chunk's face data, adding to the faces pointer if not empty
		for (Chunk::FaceAxisData& faceData : chunk->chunkFaceData) {
			const std::uint32_t totalFaces = faceData.TotalFaces<std::uint32_t>();
			if (!totalFaces) continue; // Empty - ignore
			faceDataPointers[faceDataPointersCount++] = &faceData;
			squaresCount += totalFaces;
		}
	}

	#ifndef BADCRAFT_1THREAD
	// Remove all chunks that are further than current render distance
	for (Chunk* chunk : chunksToRemove) RemoveChunk(chunk);
	chunksToRemove.clear();
	#endif

	// Bind world vertex array to edit correct buffers
	glBindVertexArray(m_worldVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_worldInstancedVBO);

	// World data buffer (on initial update, all chunk faces have instance data so the nullptr world data is not accessed)
	std::uint32_t* activeWorldData = nullptr;
	if (canMap) activeWorldData = static_cast<uint32_t*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));

	// Create new int array for all chunk data and index value
	std::uint32_t* newWorldData = new std::uint32_t[squaresCount];
	std::uint32_t newIndex = 0u;

	// If a chunk face's instance data is nullptr, it has been deleted and buffered before, so the data can instead
	// be retrieved from the current world buffer (glMapBuffer). Otherwise, buffer new data into the world array. 
	for (std::size_t i = 0; i < faceDataPointersCount; ++i) {
		Chunk::FaceAxisData* faceData = faceDataPointers[i];
		const std::uint32_t totalFaces = faceData->TotalFaces<std::uint32_t>();
		const std::size_t totalFacesBytes = sizeof(std::uint32_t) * totalFaces;

		if (faceData->instancesData == nullptr) {
			std::memcpy(newWorldData + newIndex, activeWorldData + faceData->dataIndex, totalFacesBytes);
		}
		else {
			std::memcpy(newWorldData + newIndex, faceData->instancesData, totalFacesBytes);
			delete[] faceData->instancesData;
			faceData->instancesData = nullptr;
		}

		faceData->dataIndex = newIndex;
		newIndex += totalFaces;
	}

	// Unmap array buffer only if it was mapped in the first place
	if (canMap) glUnmapBuffer(GL_ARRAY_BUFFER);

	// Buffer accumulated block data into instanced world VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_worldInstancedVBO);
	glVertexAttribIPointer(0u, 1, GL_UNSIGNED_INT, 0, nullptr);
	glBufferData(GL_ARRAY_BUFFER, sizeof(std::uint32_t) * squaresCount, newWorldData, GL_STATIC_DRAW);

	// Free memory from world data array and pointers from 'new'
	delete[] newWorldData;
	delete[] faceDataPointers;

	// Ensure new chunk data is used in indirect data and SSBO
	SortWorldBuffers();
	threadUpdateBuffers = false;
	canMap = true;
}

void World::SortWorldBuffers() noexcept
{
	// The total amount of 'chunk faces' as seen in the 'world buffer update' function
	const std::size_t chunkFacesTotal = chunks.size() * static_cast<std::size_t>(6);

	// Two per chunk face is needed to render translucent objects seperately
	IndirectDrawCommand* worldIndirectData = new IndirectDrawCommand[chunkFacesTotal * static_cast<std::size_t>(2)];
	ShaderChunkOffset* worldOffsetData = new ShaderChunkOffset[chunkFacesTotal * static_cast<std::size_t>(2)];

	// Offset value data
	ShaderChunkOffset offsetData {};
	m_indirectCalls = 0;

	struct ChunkTranslucentData {
		ShaderChunkOffset offsetData;
		Chunk* chunk;
	};

	// All chunk face data with non-opaque faces go here for sorting later on
	ChunkTranslucentData* translucentChunks = new ChunkTranslucentData[chunkFacesTotal];
	std::size_t translucentChunksCount = 0u;

	// After storing the normal face data for every chunk, loop through the individual chunk faces
	// that have translucent faces, giving the offset and chunk data for each
	for (const auto& [offset, chunk] : chunks) {
		// Set offset data for shader
		offsetData.worldPositionX = static_cast<double>(offset.x * ChunkSettings::PCHUNK_SIZE);
		offsetData.worldPositionY = static_cast<double>(offset.y * ChunkSettings::PCHUNK_SIZE);
		offsetData.worldPositionZ = static_cast<double>(offset.z * ChunkSettings::PCHUNK_SIZE);

		// TODO: frustum culling

		for (offsetData.faceIndex = 0; offsetData.faceIndex < 6; ++offsetData.faceIndex) {
			const Chunk::FaceAxisData& faceData = chunk->chunkFaceData[offsetData.faceIndex];
			if (!faceData.TotalFaces<std::uint32_t>()) continue; // No faces present

			// Add to vector if transparency is present, go to next iteration if there are no faces at all
			if (faceData.translucentFaceCount > 0u) {
				ChunkTranslucentData& translucentData = translucentChunks[translucentChunksCount++];
				translucentData.chunk = chunk;
				translucentData.offsetData = offsetData;
			}

			// Check if it would even be possible to see this (opaque) face of the chunk
			// e.g. you can't see forward faces when looking north at a chunk

			//	2d example:		---------
			//	 				|opaque |
			//	 O ----------->	| shape | <---- impossible to see this face on the opposite side
			//	/|\				|		|
			//	/ \				---------

			switch (offsetData.faceIndex) {
				case IWorldDir_Up:
					if (player.offset.y < offset.y) continue;
					break;
				case IWorldDir_Down:
					if (player.offset.y > offset.y) continue;
					break;
				case IWorldDir_Right:
					if (player.offset.x < offset.x) continue;
					break;
				case IWorldDir_Left:
					if (player.offset.x > offset.x) continue;
					break;
				case IWorldDir_Front:
					if (player.offset.z < offset.z) continue;
					break;
				case IWorldDir_Back:
					if (player.offset.z > offset.z) continue;
					break;
				default:
					break;
			}
			
			// Set indirect and offset data in the same indexes in both buffers
			worldIndirectData[m_indirectCalls] = { 4u, faceData.faceCount, 0u, faceData.dataIndex };
			worldOffsetData[m_indirectCalls++] = offsetData;
		}
	}

	// Sort chunks with transparency based on distance to camera for correct rendering order
	std::sort(
		translucentChunks,
		translucentChunks + translucentChunksCount,
		[&](const ChunkTranslucentData& a, const ChunkTranslucentData& b) {
			return a.chunk->positionMagnitude - player.posMagnitude > b.chunk->positionMagnitude - player.posMagnitude;
		}
	);

	// Loop through all of the sorted chunk faces with translucent faces and save the specific data
	for (std::size_t i = 0; i < translucentChunksCount; ++i) {
		// Get chunk face data
		const ChunkTranslucentData& data = translucentChunks[i];
		const Chunk::FaceAxisData& faceData = data.chunk->chunkFaceData[data.offsetData.faceIndex];

		// Create indirect command with offset using the translucent face data
		worldIndirectData[m_indirectCalls] = { 4u, faceData.translucentFaceCount, 0u, faceData.dataIndex + faceData.faceCount };
		worldOffsetData[m_indirectCalls++] = data.offsetData;
	}

	// Bind world vertex array to edit correct buffers
	glBindVertexArray(m_worldVAO);
	
	// Update SSBO and indirect buffer with their respective data and sizes
	glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(IndirectDrawCommand) * m_indirectCalls, worldIndirectData);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ShaderChunkOffset) * m_indirectCalls, worldOffsetData);

	// Free up memory used by arrays created with new
	delete[] translucentChunks;
	delete[] worldIndirectData;
	delete[] worldOffsetData;
}

World::~World() noexcept
{
	// Bind vertex array to get correct buffers
	glBindVertexArray(m_worldVAO);

	// Get the IDs of unnamed buffers
	GLint SSBOID, DIBID;
	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &SSBOID);
	glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &DIBID);

	// Delete created buffer objects
	const GLuint deleteBuffers[] = { 
		static_cast<GLuint>(SSBOID), 
		static_cast<GLuint>(DIBID), 
		static_cast<GLuint>(m_worldInstancedVBO), 
		static_cast<GLuint>(m_worldPlaneVBO),
	};
	glDeleteBuffers(sizeof(deleteBuffers) / sizeof(GLuint), deleteBuffers);

	// Delete world VAO
	const GLuint uintVAO = static_cast<GLuint>(m_worldVAO);
	glDeleteVertexArrays(1, &uintVAO);
}
