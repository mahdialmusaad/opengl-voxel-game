#include "World.hpp"

World::World(PlayerObject &player) noexcept : player(player)
{
	TextFormat::log("World enter");

	// VAO and VBO for debug chunk borders
	m_bordersVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);

	// Coordinates of cube edges
	const float cszf = ChunkSettings::CHUNK_SIZE_FLT;
	const float cubeLineCoordinates[] = {
		0.0f, 0.0f, 0.0f, // Bottom left back
		cszf, 0.0f, 0.0f, // Bottom right back
		0.0f, 0.0f, cszf, // Bottom left front
		cszf, 0.0f, cszf, // Bottom right front
		0.0f, cszf, 0.0f, // Top left back
		cszf, cszf, 0.0f, // Top right back
		0.0f, cszf, cszf, // Top left front
		cszf, cszf, cszf, // Top right front
	};

	m_bordersVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(cubeLineCoordinates), cubeLineCoordinates, 0u);

	// Chunk outline line indices
	const std::uint8_t cubeLineIndices[] = {
		0u, 1u,  0u, 2u,  1u, 3u,  2u, 3u,
		0u, 4u,  1u, 5u,  2u, 6u,  3u, 7u,
		4u, 5u,  4u, 6u,  5u, 7u,  6u, 7u
	};
	m_bordersEBO = OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeLineIndices), cubeLineIndices, 0u);

	// Get location of shader chunk position uniform variable
	game.shader.UseShader(Shader::ShaderID::Border);
	m_borderUniformLocation = game.shader.GetLocation(Shader::ShaderID::Border, "chunkPos");

	// Create VAO for the world buffers to store buffer settings
	m_worldVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);
	glVertexAttribDivisor(0u, 1u);

	glEnableVertexAttribArray(1u);
	glEnableVertexAttribArray(2u);
	glEnableVertexAttribArray(3u);

	// World data buffer for instanced compressed face data
	m_worldInstancedVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribIPointer(0u, 1, GL_UNSIGNED_INT, 0, nullptr);

	// Very important definition of a plane/quad for block rendering
	// Laid out like this to allow for swizzling (e.g. verticesXZ.xyz) which is 'essentially free' in shaders
	// See https://www.khronos.org/opengl/wiki/GLSL_Optimizations for more information

	const float planeVertices[] = {
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

	// TODO (possible): noise splines for more varied terrain generation
	game.noiseGenerators = WorldNoise(nullptr);

	// Initial update and buffer sizing
	UpdateRenderDistance(chunkRenderDistance);

	TextFormat::log("World exit");
}

void World::DrawWorld() noexcept
{
	// Use correct VAO and shader program
	game.shader.UseShader(Shader::ShaderID::Blocks);
	glBindVertexArray(m_worldVAO);

	// Draw the entire world in a single draw call using the indirect buffer, essentially
	// doing an instanced draw call (glDrawArraysInstancedBaseInstance) for each 'chunk face'.
	// See https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMultiDrawArraysIndirect.xhtml for more information.

	glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, nullptr, m_indirectCalls, 0);
}

void World::DebugChunkBorders(bool drawing) noexcept
{
	if (!game.chunkBorders) return; // Only render/update if enabled

	// Use borders VAO and shader for correct data editing/usage
	game.shader.UseShader(Shader::ShaderID::Border);
	glBindVertexArray(m_bordersVAO);
	
	// Draw lines using indexed line data
	if (drawing) glDrawElements(GL_LINES, 24, GL_UNSIGNED_BYTE, nullptr);
	else {
		// Update chunk position in shader
		const glm::dvec3 chunkPos = player.offset * static_cast<PosType>(ChunkSettings::CHUNK_SIZE);
		glUniform3d(m_borderUniformLocation, chunkPos.x, chunkPos.y, chunkPos.z);
	}
}

void World::DebugReset() noexcept
{
	// For debugging purposes - regenerate all nearby chunks
	std::vector<Chunk*> toRemove;
	for (auto it = allchunks.begin(); it != allchunks.end(); ++it) toRemove.emplace_back(it->second);
	for (Chunk *c : toRemove) RemoveChunk(c);
	OffsetUpdate();
}

Chunk *World::WorldPositionToChunk(const WorldPos &pos) const noexcept
{
	// Gets the chunk that contains the given world position
	const glm::dvec3 dpos = static_cast<glm::dvec3>(pos);
	return GetChunk(ChunkSettings::WorldToOffset(dpos));
}

ObjectID World::GetBlock(const WorldPos &pos) const noexcept
 {
	Chunk *chunk = WorldPositionToChunk(pos); // Get the chunk that contains the given position
	if (chunk && chunk->chunkBlocks) return chunk->chunkBlocks->at(ChunkSettings::WorldToLocal(pos)); // Returns the block at the local position in the chunk
	else return ObjectID::Air; // If no chunk is found, return an air block
}

void World::SetBlock(const WorldPos &pos, ObjectID block, bool updateChunk) noexcept
{
	// Get chunk that contains the given position
	Chunk *chunk = WorldPositionToChunk(pos);

	// If it exists, add the block change and do the necessary updates
	if (chunk) {
		if (!chunk->chunkBlocks) {
			if (block == ObjectID::Air) return; // Ignore uneccessary changes (air to 'air chunk')
			else chunk->AllocateChunkBlocks(); // Use normal block storage
		}

		const glm::ivec3 localPos = ChunkSettings::WorldToLocal(pos);

		// Update bordering chunks if changed block was on a corner
		NearbyChunkData nearbyData[6];
		const int count = GetNearbyChunks(chunk, nearbyData, true);
		for (int i = 0; i < count; ++i) {
			const NearbyChunkData &nearby = nearbyData[i];
			if (!ChunkSettings::IsOnCorner(localPos, nearby.direction)) continue;
			if (updateChunk) nearby.nearbyChunk->CalculateTerrainData(allchunks);
			else nearby.nearbyChunk->gameState = Chunk::ChunkState::UpdateRequest;
		}

		chunk->chunkBlocks->atref(localPos) = block; // Change block at local position

		if (updateChunk) { chunk->CalculateTerrainData(allchunks); UpdateWorldBuffers(); }
		else chunk->gameState = Chunk::ChunkState::UpdateRequest;
	}

	// Do nothing if the chunk does not exist
}

bool World::FillBlocks(WorldPos from, WorldPos to, ObjectID objectID) noexcept
{
	// Force valid position - ensure Y position is in range
	from.y = glm::clamp(from.y, static_cast<PosType>(0), static_cast<PosType>(ChunkSettings::MAX_WORLD_HEIGHT));
	to.y = glm::clamp(to.y, static_cast<PosType>(0), static_cast<PosType>(ChunkSettings::MAX_WORLD_HEIGHT));

	// Determine direction to move in each axis
	const WorldPos step = {
		static_cast<PosType>(from.x > to.x ? -1 : 1),
		static_cast<PosType>(from.y > to.y ? -1 : 1),
		static_cast<PosType>(from.z > to.z ? -1 : 1)
	};

	// Prevent extremely large changes
	const PosType totalSets = glm::abs(to.x - from.x) * glm::abs(to.y - from.y) * glm::abs(to.z - from.z);
	if (totalSets > static_cast<PosType>(20000)) return false;

	// If the from-to values are equal, it should still place a block
	for (int i = 0; i < 3; ++i) if (from[i] == to[i]) from[i] -= step[i];
	
	// Set all of the valid blocks from fx, fy, fz to tx, ty, tz (inclusive) as the given 'block ID'
	for (PosType fx = from.x; fx != to.x; fx += step.x) {
		for (PosType fy = from.y; fy != to.y; fy += step.y) {
			for (PosType fz = from.z; fz != to.z; fz += step.z) {
				const WorldPos cfrom = { fx, fy, fz };
				SetBlock(cfrom, objectID, false);
			}
		}
	}

	// Update all affected chunks
	for (const auto &it : m_blockQueue) ApplyQueue(it.second, it.first, true);
	ApplyUpdateRequest();
	UpdateWorldBuffers();

	return true;
}

Chunk *World::GetChunk(const WorldPos &chunkOffset) const noexcept
{
	const auto it = allchunks.find(chunkOffset); // Find chunk with given offset key
	return it != allchunks.end() ? it->second : nullptr; // Return chunk if it was found
}

PosType World::HighestBlockPosition(PosType x, PosType z) const noexcept
{
	const PosType offsetX = ChunkSettings::WorldToOffset(x), offsetZ = ChunkSettings::WorldToOffset(z); // Get chunk offsets containing XZ position
	glm::ivec3 chunkPos = { ChunkSettings::WorldToLocal(x), 0, ChunkSettings::WorldToLocal(z) }; // Get the local chunk position from the world position

	for (int y = ChunkSettings::HEIGHT_COUNT - 1; y >= 0; --y) {
		Chunk *chunk = GetChunk({ offsetX, y, offsetZ });
		const PosType worldY = y * ChunkSettings::CHUNK_SIZE;
		if (chunk && chunk->chunkBlocks) {
			// Search the local XZ coordinate inside found chunk from top to bottom
			for (chunkPos.y = ChunkSettings::CHUNK_SIZE_M1; chunkPos.y >= 0; --chunkPos.y) {
				if (chunk->chunkBlocks->at(chunkPos) != ObjectID::Air) return worldY + static_cast<PosType>(chunkPos.y);
			}
		}
	}

	// Fallback to bottom position
	return static_cast<PosType>(0);
}

bool World::InRenderDistance(WorldPos &playerOffset, const WorldPos &chunkOffset) noexcept
{
	// Check if distance from each X + Z positions in both values together are less than the render distance
	return glm::abs(playerOffset.x - chunkOffset.x) +
		   glm::abs(playerOffset.z - chunkOffset.z) <= static_cast<PosType>(chunkRenderDistance);
}

void World::UpdateRenderDistance(int newRenderDistance) noexcept
{
	// Render distance determines how many chunks in a 'star' pattern will be generated
	// from the initial player chunk (e.g. a render distance of 0 has only the player's chunk,
	// whereas a render distance of 1 has 4 extra chunks surrounding it - 5 in total).

	// Prevent negative render distance
	if (newRenderDistance < 0) return;
	chunkRenderDistance = static_cast<std::int32_t>(newRenderDistance);
	
	// Maximum amount of chunk faces, calculated as ( (2 * n * n) + (2 * n) + 1 ) * h, 
	// where n is the render distance and h is the number of chunk *faces* in a full chunk -> HEIGHT_COUNT * 6
	const int surroundingOffsetsAmount = GetNumChunks(false);
	const std::size_t maxChunkFaces = surroundingOffsetsAmount * ChunkSettings::HEIGHT_COUNT * 6u;
	
	// Create arrays for face data and chunk sorting with new sizes
	faceDataPointers = new Chunk::FaceAxisData*[maxChunkFaces];
	translucentChunks = new ChunkTranslucentData[maxChunkFaces];
	
	// Arrays for indirect and chunk offset data on camera/player move -
	// multiply size by 2 due to chunk faces with transparency needing to be rendered seperately
	const std::size_t maxChunkFacesTransparency = maxChunkFaces * 2u;
	worldIndirectData = new IndirectDrawCommand[maxChunkFacesTransparency];
	worldOffsetData = new ShaderChunkFace[maxChunkFacesTransparency];

	// Determine surrounding relative chunk offsets for generation
	if (surroundingOffsets) delete[] surroundingOffsets;
	surroundingOffsets = new ChunkOffset[surroundingOffsetsAmount];
	int crd = chunkRenderDistance, sInd = 0;
	for (int x = -crd; x <= crd; ++x) for (int z = -crd; z <= crd; ++z) if (glm::abs(x) + glm::abs(z) <= crd) surroundingOffsets[sInd++] = ChunkOffset(x, z);

	glBindVertexArray(m_worldVAO); // Ensure correct buffers are updated
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectDrawCommand) * maxChunkFacesTransparency, nullptr, GL_STATIC_DRAW); // Indirect buffer - storage for draw commands
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ShaderChunkFace) * maxChunkFacesTransparency, nullptr, GL_STATIC_COPY); // SSBO - shader data storage (chunk offset + face index)

	TextFormat::log(fmt::format("Render distance changed to {}", chunkRenderDistance));

	// Fill the newly sized buffers (empty) with new valid data
	OffsetUpdate();
}

void World::RemoveChunk(Chunk *chunk) noexcept
{
	// Remove from map and free up memory using delete as it was created with 'new'
	allchunks.erase({ *chunk->offset });
	delete chunk;
}

void World::SetPerlinValues(WorldPerlin::NoiseResult *results, ChunkOffset offset) noexcept
{
	// Used per full chunk, each chunk would have the same results as they have 
	// the same XZ coordinates so no calculation is needed for each individual chunk

	// Loop through X and Z axis
	for (int i = 0; i < ChunkSettings::CHUNK_SIZE_SQUARED; ++i) {
		const int indZ = i / ChunkSettings::CHUNK_SIZE, indX = i % ChunkSettings::CHUNK_SIZE; // Get the X and Z position
		// Get noise coordinates
		const double posX = static_cast<double>(offset.x + indX) * ChunkSettings::NOISE_STEP, posZ = static_cast<double>(offset.y + indZ) * ChunkSettings::NOISE_STEP;

		// Store the height result for each of the terrain noise generators
		WorldPerlin::NoiseResult &result = results[i];

		result.continentalnessHeight = ((game.noiseGenerators.continentalness.Octave2D(posX, posZ, 3) + 0.4f) * ChunkSettings::NOISE_MULTIPLIER) + 20.0f;
		result.flatness = game.noiseGenerators.temperature.Noise2D(posX, posZ);
		result.temperature = game.noiseGenerators.flatness.Noise2D(posX, posZ);
		result.humidity = game.noiseGenerators.humidity.Noise2D(posX, posZ);
	}
}

void World::ApplyQueue(Chunk *chunk, const BlockQueueVector &blockQueue, bool calculate) noexcept
{
	// Ignore empty vectors
	if (!blockQueue.size()) return;

	// Convert air chunk to normal per-block storage if needed
	chunk->AllocateChunkBlocks();

	// Apply queue whilst checking if certain blocks are replaceable depending on strength
	// (only if the change is considered 'natural', such as trees)
	for (const Chunk::BlockQueue &qBlock : blockQueue) {
		ObjectID &currentBlock = chunk->chunkBlocks->atref(qBlock.pos);
		if (qBlock.natural) {
			const WorldBlockData &currentBlockData = ChunkSettings::GetBlockData(currentBlock);
			const WorldBlockData &replaceBlockData = ChunkSettings::GetBlockData(qBlock.blockID);
			if (currentBlockData.strength > replaceBlockData.strength) continue;
		}
		currentBlock = qBlock.blockID;
	}

	// Remove block queue for this chunk
	m_blockQueue.erase(*chunk->offset);

	// Calculate and/or update buffers if requested
	if (calculate) chunk->CalculateTerrainData(allchunks);
}

bool World::ApplyQueue(const BlockQueueVector &blockQueue, const WorldPos &offset, bool calculate) noexcept
{
	Chunk *chunk = GetChunk(offset); // Get chunk from given offset
	if (!chunk) return false; // Check if it exists
	ApplyQueue(chunk, blockQueue, calculate); // Change chunk blocks using given queue vector
	return true;
}

bool World::ApplyQueue(Chunk *chunk, bool calculate) noexcept
{
	const auto &foundQueue = m_blockQueue.find(*chunk->offset); // Find queue vector from chunk offset
	if (foundQueue == m_blockQueue.end()) return false; // Check if it exists
	ApplyQueue(chunk, foundQueue->second, calculate); // Change chunk blocks using queue vector
	return true;
}

void World::ApplyUpdateRequest() noexcept
{
	bool updated = false;

	// Calculate any marked chunks
	for (const auto &it : allchunks) {
		if (it.second->gameState == Chunk::ChunkState::UpdateRequest) {
			it.second->CalculateTerrainData(allchunks);
			updated = true;
		}
	}

	// Update buffers to show changes
	if (updated) UpdateWorldBuffers();
}

void World::OffsetUpdate() noexcept
{
	// TODO: possibly include the chunks around deleted chunks in calculation

	// Remove any chunks further than the render distance
	std::vector<Chunk*> chunkVector;
	for (const auto &it : allchunks) if (!InRenderDistance(player.offset, it.first)) chunkVector.emplace_back(it.second);
	for (Chunk *p : chunkVector) RemoveChunk(p);
	chunkVector.clear();

	// Find valid coordinates relative to the player, creating a new full chunk if there isn't one already
	const ChunkOffset playerOffset = { player.offset.x, player.offset.z };
	for (int i = 0, max = GetNumChunks(false); i < max; ++i) {
		const ChunkOffset newOffset = playerOffset + surroundingOffsets[i];
		const WorldPos newFullOffset = { newOffset.x, 0, newOffset.y };
		if (allchunks.find(newFullOffset) != allchunks.end()) continue;

		// Precalculate the noise values for terrain generation
		SetPerlinValues(noiseResults, newOffset * static_cast<PosType>(ChunkSettings::CHUNK_SIZE));

		// Create a new chunk at each Y offset, add it to the chunk vector and set bottom nearby chunk if possible
		WorldPos offset = { newOffset.x, static_cast<PosType>(0), newOffset.y };
		for (; offset.y < static_cast<PosType>(ChunkSettings::HEIGHT_COUNT); ++offset.y) {
			Chunk *newChunk = new Chunk(offset);
			newChunk->offset = &allchunks.insert({ offset, newChunk }).first->first;
			newChunk->ConstructChunk(noiseResults, m_blockQueue);
			chunkVector.emplace_back(newChunk);
		}
	}
	
	// Store unique chunks requiring calculation
	Chunk::WorldMapDef uniqueChunks;
	// Store direction and pointer to nearby chunks
	NearbyChunkData nearbyData[4];
	
	// Add surrounding chunks to unique map - do after chunk creation to include all chunks when searching
	for (Chunk *chunk : chunkVector) {
		uniqueChunks[*chunk->offset] = chunk;
		const int numSurrounding = GetNearbyChunks(chunk, nearbyData, false);
		for (int i = 0; i < numSurrounding; ++i) {
			Chunk *nearbyChunk = nearbyData[i].nearbyChunk;
			uniqueChunks[*nearbyChunk->offset] = nearbyChunk;
		}
	}
	
	// Calculate chunks that are in the 'unique' map and apply any queued blocks for them
	for (const auto &it : uniqueChunks) if (!ApplyQueue(it.second, true)) it.second->CalculateTerrainData(allchunks);
	
	// Update world buffers to use calculated chunk data
	UpdateWorldBuffers();
}

void World::UpdateWorldBuffers() noexcept
{
	if (!allchunks.size()) {
		TextFormat::warn("Updating buffers with no chunks", "Empty update");
		return;
	}

	// Counters for faces and face data pointer
	std::size_t faceDataPointersCount = 0u;
	squaresCount = 0u;

	// Loop through all of the chunks and each of their 6 face data to determine how much memory is needed
	// overall and accumulate all the valid pointer data into the array, as well as preparing to delete any far away chunks
	for (auto it = allchunks.begin(); it != allchunks.end(); ++it) {
		// Loop through all of the chunk's face data, adding to the faces pointer if not empty
		for (Chunk::FaceAxisData &faceData : it->second->chunkFaceData) {
			const std::uint32_t totalFaces = faceData.TotalFaces<std::uint32_t>();
			if (!totalFaces) continue; // Empty - ignore
			faceDataPointers[faceDataPointersCount++] = &faceData;
			squaresCount += totalFaces;
		}
	}

	// Bind world vertex array to edit correct buffers
	glBindVertexArray(m_worldVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_worldInstancedVBO);

	// World data buffer (on initial update, all chunk faces have instance data so the nullptr world data is not accessed)
	std::uint32_t *activeWorldData = nullptr;
	if (canMap) activeWorldData = static_cast<std::uint32_t*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));

	// Create new int array for all chunk data and index value
	std::uint32_t *newWorldData = new std::uint32_t[squaresCount];
	std::uint32_t newIndex = 0u;

	// If a chunk face's instance data does not exist, it has been deleted and buffered before so the data can instead
	// be retrieved from the current world buffer (glMapBuffer). Otherwise, buffer the new data into the world array. 
	for (std::size_t i = 0; i < faceDataPointersCount; ++i) {
		Chunk::FaceAxisData *faceData = faceDataPointers[i];
		const std::uint32_t totalFaces = faceData->TotalFaces<std::uint32_t>();
		const std::size_t totalFacesBytes = sizeof(std::uint32_t) * totalFaces;

		if (!faceData->instancesData) {
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

	// Free memory from world data array
	delete[] newWorldData;

	// Ensure new chunk data is used in indirect data and SSBO
	SortWorldBuffers();
	canMap = true;
}

void World::SortWorldBuffers() noexcept
{
	// Offset value data
	ShaderChunkFace offsetData {};
	m_indirectCalls = 0;

	// Counter for translucent chunk data array
	std::size_t translucentChunksCount = 0u;

	// After storing the normal face data for every chunk, loop through the individual chunk faces
	// that have translucent faces, giving the offset and chunk data for each
	for (auto it = allchunks.begin(); it != allchunks.end(); ++it) {
		const WorldPos &offset = it->first;
		Chunk *chunk = it->second;

		// Set offset data for shader
		offsetData.worldPositionX = static_cast<double>(offset.x * static_cast<PosType>(ChunkSettings::CHUNK_SIZE));
		offsetData.worldPositionZ = static_cast<double>(offset.z * static_cast<PosType>(ChunkSettings::CHUNK_SIZE));
		offsetData.worldPositionY = static_cast<float> (offset.y * static_cast<PosType>(ChunkSettings::CHUNK_SIZE));

		for (offsetData.faceIndex = 0; offsetData.faceIndex < 6; ++offsetData.faceIndex) {
			const Chunk::FaceAxisData &faceData = chunk->chunkFaceData[offsetData.faceIndex];
			if (!faceData.TotalFaces<std::uint32_t>()) continue; // No faces present

			// Add to vector if transparency is present, go to next iteration if there are no faces at all
			if (faceData.translucentFaceCount > 0u) {
				ChunkTranslucentData &translucentData = translucentChunks[translucentChunksCount++];
				translucentData.chunk = chunk;
				translucentData.offsetData = offsetData;
			}

			// Check if it would even be possible to see this (opaque) face of the chunk
			// e.g. you can't see forward faces when looking north at a chunk
			
			switch (offsetData.faceIndex) {
				case WldDir_Right:
					if (player.offset.x < offset.x) continue;
					break;
				case WldDir_Left:
					if (player.offset.x > offset.x) continue;
					break;
				case WldDir_Up:
					if (player.offset.y < offset.y) continue;
					break;
				case WldDir_Down:
					if (player.offset.y > offset.y) continue;
					break;
				case WldDir_Front:
					if (player.offset.z < offset.z) continue;
					break;
				case WldDir_Back:
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

	// Loop through all of the sorted chunk faces with translucent faces and save the specific data
	for (std::size_t i = 0; i < translucentChunksCount; ++i) {
		// Get chunk face data
		const ChunkTranslucentData &data = translucentChunks[i];
		const Chunk::FaceAxisData &faceData = data.chunk->chunkFaceData[data.offsetData.faceIndex];

		// Create indirect command with offset using the translucent face data
		worldIndirectData[m_indirectCalls] = { 4u, faceData.translucentFaceCount, 0u, faceData.dataIndex + faceData.faceCount };
		worldOffsetData[m_indirectCalls++] = data.offsetData;
	}

	// Bind world vertex array to edit correct buffers
	glBindVertexArray(m_worldVAO);
	
	// Update SSBO and indirect buffer with their respective data and sizes
	glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(IndirectDrawCommand) * m_indirectCalls, worldIndirectData);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ShaderChunkFace) * m_indirectCalls, worldOffsetData);
}

int World::GetNearbyChunks(Chunk *chunk, NearbyChunkData *nearbyData, bool includeY)
{
	int directionIndex = 0;
	int count = 0;

	const auto AddNearbyChunk = [&](const WorldPos &direction) {
		const auto &foundChunk = allchunks.find(*chunk->offset + direction);
		if (foundChunk != allchunks.end()) {
			NearbyChunkData ncd;
			ncd.direction = static_cast<WorldDirection>(directionIndex);
			ncd.nearbyChunk = foundChunk->second;
			nearbyData[count++] = ncd;
		}
	};

	for (const WorldPos &direction : game.constants.worldDirections) {
		if (!includeY && (directionIndex == WldDir_Up || directionIndex == WldDir_Down));
		else AddNearbyChunk(direction);
		directionIndex++;
	}

	return count;
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
		m_worldInstancedVBO, 
		m_worldPlaneVBO,
	};
	glDeleteBuffers(static_cast<GLsizei>(Math::size(deleteBuffers)), deleteBuffers);

	// Delete world VAO
	const GLuint uintVAO = static_cast<GLuint>(m_worldVAO);
	glDeleteVertexArrays(1, &uintVAO);

	// Delete stored arrays
	delete[] faceDataPointers;
	delete[] surroundingOffsets;
	delete[] translucentChunks;
	delete[] worldIndirectData;
	delete[] worldOffsetData;
	delete[] noiseResults;
}
