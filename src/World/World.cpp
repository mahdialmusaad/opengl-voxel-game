#include "World.hpp"

World::World(WorldPlayer &player) noexcept : player(player)
{
	// VAO and VBO for debug chunk borders
	m_bordersVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);

	// Coordinates of cube edges
	const float cszf = static_cast<float>(ChunkValues::size);
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
	m_borderUniformLocation = game.shaders.programs.border.GetLocation("chunkPos");

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
	// Laid out like this to allow for swizzling (e.g. verticesXZ.xyz) which is "essentially free" in shaders
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
	m_worldSSBO = OGL::CreateBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0u, m_worldSSBO);
	// Buffer which holds the indexes into the world data for each 'instanced draw call' in indirect draw call
	m_worldIBO = OGL::CreateBuffer(GL_DRAW_INDIRECT_BUFFER);

	// TODO (possible): noise splines for more varied terrain generation
	game.noiseGenerators = WorldNoise(nullptr);

	// Initial update and buffer sizing
	UpdateRenderDistance(chunkRenderDistance);
}

void World::DrawWorld() const noexcept
{
	// Use correct VAO and shader program
	game.shaders.programs.blocks.Use();
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
	game.shaders.programs.border.Use();
	glBindVertexArray(m_bordersVAO);
	
	// Draw lines using indexed line data
	if (drawing) glDrawElements(GL_LINES, 24, GL_UNSIGNED_BYTE, nullptr);
	else {
		// Update chunk position in shader
		const glm::dvec3 chunkPos = player.offset * static_cast<PosType>(ChunkValues::size);
		glUniform3dv(m_borderUniformLocation, 1, &chunkPos[0]);
	}
}

void World::DebugReset() noexcept
{
	// For debugging purposes - regenerate all nearby chunks
	for (auto it = allchunks.cbegin(); it != allchunks.cend();) { delete it->second; allchunks.erase(it++); }
	OffsetUpdate();
}

Chunk *World::WorldPositionToChunk(const WorldPosition &pos) const noexcept
{
	// Gets the chunk that contains the given world position
	return GetChunk(ChunkValues::WorldToOffset(pos));
}

ObjectID World::GetBlock(const WorldPosition &pos) const noexcept
 {
	Chunk *chunk = WorldPositionToChunk(pos); // Get the chunk that contains the given position
	if (chunk && chunk->chunkBlocks) return chunk->chunkBlocks->at(ChunkValues::WorldToLocal(pos)); // Returns the block at the local position in the chunk
	else return ObjectID::Air; // If no chunk is found, return an air block
}

void World::SetBlock(const WorldPosition &pos, ObjectID block, bool updateChunk) noexcept
{
	// Get chunk that contains the given position
	const WorldPosition offset = ChunkValues::WorldToOffset(pos);
	Chunk *chunk = GetChunk(offset);

	// Get local chunk position of the block
	const glm::ivec3 localPos = ChunkValues::WorldToLocal(pos);

	// Add to queue if the chunk does not exist, no need to check for bordering chunks
	// as they will be updated later on when the chunk is created
	if (!chunk) {
		m_blockQueue[offset].emplace_back(Chunk::BlockQueue(localPos, block, false));
		return;
	}

	// If it exists, change the block and possibly update chunk + bordering chunks
	if (!chunk->chunkBlocks) {
		if (block == ObjectID::Air) return; // Ignore uneccessary changes (air to 'air chunk')
		else chunk->AllocateChunkBlocks(); // Use normal block storage
	}

	chunk->chunkBlocks->atref(localPos) = block; // Change block at local position

	// Update bordering chunks if changed block was on a corner
	NearbyChunkData nearbyData[6];
	const int count = GetNearbyChunks(offset, nearbyData, true);
	for (int i = 0; i < count; ++i) {
		const NearbyChunkData &nearby = nearbyData[i];
		if (!ChunkValues::IsOnCorner(localPos, nearby.direction)) continue;
		if (updateChunk) nearby.nearbyChunk->CalculateTerrainData(allchunks);
		else nearby.nearbyChunk->gameState = Chunk::ChunkState::UpdateRequest;
	}
	
	if (updateChunk) { chunk->CalculateTerrainData(allchunks); UpdateWorldBuffers(); }
	else chunk->gameState = Chunk::ChunkState::UpdateRequest;
}

std::uintmax_t World::FillBlocks(WorldPosition from, WorldPosition to, ObjectID objectID) noexcept
{
	// Force valid position - ensure Y position is in range
	from.y = glm::clamp(from.y, PosType{}, static_cast<PosType>(ChunkValues::maxHeight));
	to.y = glm::clamp(to.y, PosType{}, static_cast<PosType>(ChunkValues::maxHeight));

	// Determine direction to move in each axis
	const WorldPosition step = {
		static_cast<PosType>(from.x > to.x ? -1 : 1),
		static_cast<PosType>(from.y > to.y ? -1 : 1),
		static_cast<PosType>(from.z > to.z ? -1 : 1)
	};
	
	// Prevent extremely large changes
	const std::uintmax_t totalSets = glm::abs(to.x - from.x) * glm::abs(to.y - from.y) * glm::abs(to.z - from.z);
	if (totalSets > static_cast<std::uintmax_t>(32768)) return totalSets;

	// Ensure a block is still placed if any value is equal
	for (int i = 0; i < 3; ++i) {
		PosType &toAxis = to[i];
		if (toAxis == from[i]) toAxis += step[i];
	}

	// Set all of the valid blocks from fx, fy, fz to tx, ty, tz (inclusive) as the given block ID
	WorldPosition set{};
	for (set.x = from.x; set.x != to.x; set.x += step.x) {
		for (set.y = from.y; set.y != to.y; set.y += step.y) {
			for (set.z = from.z; set.z != to.z; set.z += step.z) SetBlock(set, objectID, false);
		}
	}

	// Update all affected chunks
	for (const auto &it : m_blockQueue) ApplyQueue(it.second, it.first, true);
	ApplyUpdateRequest();
	UpdateWorldBuffers();

	return std::uintmax_t{};
}

Chunk *World::GetChunk(const WorldPosition &chunkOffset) const noexcept
{
	const auto it = allchunks.find(chunkOffset); // Find chunk with given offset key
	return it != allchunks.end() ? it->second : nullptr; // Return chunk if it was found
}

PosType World::HighestBlockPosition(PosType x, PosType z) const noexcept
{
	const PosType offsetX = ChunkValues::WorldToOffset(x), offsetZ = ChunkValues::WorldToOffset(z); // Get chunk offsets containing XZ position
	glm::ivec3 chunkPos = { ChunkValues::WorldToLocal(x), 0, ChunkValues::WorldToLocal(z) }; // Get the local chunk position from the world position

	for (int y = ChunkValues::heightCount - 1; y >= 0; --y) {
		Chunk *chunk = GetChunk({ offsetX, y, offsetZ });
		if (!chunk || !chunk->chunkBlocks) continue; // Check if it is a valid chunk with blocks
		
		// Search the local XZ coordinate inside found chunk from top to bottom
		const PosType worldY = y * ChunkValues::size;
		for (chunkPos.y = ChunkValues::sizeLess; chunkPos.y >= 0; --chunkPos.y) {
			if (chunk->chunkBlocks->at(chunkPos) != ObjectID::Air) return worldY + static_cast<PosType>(chunkPos.y);
		}
	}

	// Fallback to bottom position
	return PosType{};
}

PosType World::PlayerChunkDistance(const WorldPosition &chunkOffset) const noexcept
{
	// Returns how many chunks are in between the player and the given chunk offset
	return glm::abs(player.offset.x - chunkOffset.x) + glm::abs(player.offset.z - chunkOffset.z);
}

bool World::InRenderDistance(const WorldPosition &chunkOffset) const noexcept
{
	// Check if distance from each X + Z positions in both values together are less than the render distance
	return PlayerChunkDistance(chunkOffset) <= static_cast<PosType>(chunkRenderDistance);
}

void World::UpdateRenderDistance(int newRenderDistance) noexcept
{
	// Render distance determines how many chunks in a 'star' pattern will be generated
	// from the initial player chunk (e.g. a render distance of 0 has only the player's chunk,
	// whereas a render distance of 1 has 4 extra chunks surrounding it - 5 in total).

	// Ensure render distance is within bounds
	if (newRenderDistance < ValueLimits::renderLimit.min) return;
	chunkRenderDistance = static_cast<std::int32_t>(newRenderDistance);
	
	// Maximum amount of chunk faces, calculated as ( (2 * n * n) + (2 * n) + 1 ) * h, 
	// where n is the render distance and h is the number of chunk *faces* in a full chunk -> HEIGHT_COUNT * 6
	const int surroundingOffsetsAmount = GetNumChunks(false);
	const std::size_t maxChunkFaces = surroundingOffsetsAmount * ChunkValues::heightCount * 6u;

	// Delete any existing arrays
	if (surroundingOffsets) delete[] surroundingOffsets;
	if (translucentChunks) delete[] translucentChunks;
	if (worldIndirectData) delete[] worldIndirectData;
	if (faceDataPointers) delete[] faceDataPointers;
	if (worldOffsetData) delete[] worldOffsetData;
	
	// Create arrays for face data and chunk sorting with new sizes
	faceDataPointers = new Chunk::FaceAxisData*[maxChunkFaces];
	translucentChunks = new ChunkTranslucentData[maxChunkFaces];
	
	// Arrays for indirect and chunk offset data
	// Multiply size by 2 due to chunk faces with transparency needing to be rendered seperately
	const std::size_t maxChunkFacesTransparency = maxChunkFaces * 2u;
	worldIndirectData = new IndirectDrawCommand[maxChunkFacesTransparency];
	worldOffsetData = new ShaderChunkFace[maxChunkFacesTransparency];

	// Determine surrounding relative chunk offsets for generation
	surroundingOffsets = new WorldXZPosition[surroundingOffsetsAmount];
	int crd = static_cast<int>(chunkRenderDistance), sInd = 0;
	for (int x = -crd; x <= crd; ++x) for (int z = -crd; z <= crd; ++z) if (glm::abs(x) + glm::abs(z) <= crd) surroundingOffsets[sInd++] = { x, z };

	// Ensure correct buffers are updated
	glBindVertexArray(m_worldVAO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_worldSSBO);

	// Storage for draw commands
	glBufferData(GL_DRAW_INDIRECT_BUFFER, static_cast<GLsizeiptr>(sizeof(IndirectDrawCommand) * maxChunkFacesTransparency), nullptr, GL_STATIC_DRAW);
	// Shader data storage (chunk position and face index)
	glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(sizeof(ShaderChunkFace) * maxChunkFacesTransparency), nullptr, GL_STATIC_COPY);

	TextFormat::log(fmt::format("Render distance changed ({})", chunkRenderDistance));

	// Update chunks to use new offset radius (generate or delete)
	OffsetUpdate();
}

void World::SetPerlinValues(WorldPerlin::NoiseResult *results, WorldXZPosition chunkPos) noexcept
{
	// Used per full chunk, each chunk would have the same results as they have 
	// the same XZ coordinates so no calculation is needed for each individual chunk
	constexpr float defVal = NoiseValues::defaultZ;

	// Loop through X and Z axis
	for (int i = 0; i < ChunkValues::sizeSquared; ++i) {
		const int indZ = i / ChunkValues::size, indX = i % ChunkValues::size; // Get the X and Z position
		// Get noise coordinates from XZ positions with given offsets
		const double posX = static_cast<double>(chunkPos.x + static_cast<PosType>(indX)) * NoiseValues::noiseStep;
		const double posZ = static_cast<double>(chunkPos.y + static_cast<PosType>(indZ)) * NoiseValues::noiseStep;

		// Store the noise results for each of the terrain noise generators
		results[i] = WorldPerlin::NoiseResult(
			posX, posZ,
			(game.noiseGenerators.elevation.GetOctave(posX, defVal, posZ, 3) * NoiseValues::terrainRange) + NoiseValues::minSurface,
			game.noiseGenerators.flatness.GetNoise(posX, defVal, posZ),
			game.noiseGenerators.temperature.GetNoise(posX, defVal, posZ),
			game.noiseGenerators.humidity.GetNoise(posX, defVal, posZ)
		);
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
		if (!qBlock.natural) {
			const WorldBlockData &currentBlockData = ChunkValues::GetBlockData(currentBlock);
			const WorldBlockData &replaceBlockData = ChunkValues::GetBlockData(qBlock.blockID);
			if (currentBlockData.strength > replaceBlockData.strength) continue;
		}
		currentBlock = qBlock.blockID;
	}

	// Remove block queue for this chunk
	m_blockQueue.erase(*chunk->offset);

	// Calculate chunk terrain if requested
	if (calculate) chunk->CalculateTerrainData(allchunks);
}

bool World::ApplyQueue(const BlockQueueVector &blockQueue, const WorldPosition &offset, bool calculate) noexcept
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
		if (it.second->gameState != Chunk::ChunkState::UpdateRequest) continue;
		it.second->CalculateTerrainData(allchunks);
		updated = true;
	}

	// Update buffers to show changes
	if (updated) UpdateWorldBuffers();
}

void World::AddSurrounding(Chunk::WorldMapDef &chunksMap, NearbyChunkData *nearbyData, const WorldPosition &offset, bool inclY) const noexcept
{
	const int numSurrounding = GetNearbyChunks(offset, nearbyData, inclY); // Get number of chunks surrounding given offset
	// Add chunks surrounding given offset to given map
	for (int j = 0; j < numSurrounding; ++j) {
		Chunk *nearbyChunk = nearbyData[j].nearbyChunk;
		chunksMap[*nearbyChunk->offset] = nearbyChunk;
	}
}

void World::OffsetUpdate() noexcept
{
	Chunk::WorldMapDef affectedChunks; // Store unique affected chunks requiring calculation
	NearbyChunkData nearbyData[4]; // Store direction and pointer to nearby chunks

	for (auto it = allchunks.cbegin(); it != allchunks.cend();) {
		WorldPosition offset = it->first;
		if (InRenderDistance(offset)) { ++it; continue; }; // Check if it is further than the render distance
		delete it->second;
		allchunks.erase(it++);
	}
	
	const int numFullChunks = GetNumChunks(false);
	int newOffsetsCount = 0;

	// Player X and Z offset - use to determine new chunk offsets
	const WorldXZPosition playerOffset = { player.offset.x, player.offset.z };
	WorldXZPosition *newOffsets = new WorldXZPosition[numFullChunks]; // XZ offsets of chunks that need creating
	
	// Create a full chunk around the player if one doesn't exist already
	for (int offsetInd = 0; offsetInd < numFullChunks; ++offsetInd) {
		const WorldXZPosition newXZOffset = playerOffset + surroundingOffsets[offsetInd]; // Get XZ offset of possible chunk
		if (GetChunk({ newXZOffset.x, PosType{}, newXZOffset.y })) continue; // Check if it already exists
		newOffsets[newOffsetsCount++] = newXZOffset; // Chunks need to be created at this offset
	}

	// Number of full chunks per thread
	const int numFullChunksEach = newOffsetsCount / game.numThreads;
	int numFullChunksLeft = newOffsetsCount - (numFullChunksEach * game.numThreads); // Size may not be a multiple of threads count
	const int chunkArrayLen = newOffsetsCount * ChunkValues::heightCount;

	Chunk **chunkArray = new Chunk*[chunkArrayLen]; // Array of newly created chunks

	for (int thread = 0, threadIndex = 0; thread < game.numThreads; ++thread) {
		const int start = threadIndex;
		threadIndex += numFullChunksEach + (numFullChunksLeft-- > 0 ? 1 : 0); // Spread leftover offsets over threads

		// Create the full chunks in parallel
		game.genThreads[thread] = std::thread([&](int mapInd, int offsetStart, int offsetsEnd) {
			Chunk::BlockQueueMap &threadMap = threadMaps[mapInd];
			int chunksStart = offsetStart * ChunkValues::heightCount;
			WorldPerlin::NoiseResult* noiseResults = new WorldPerlin::NoiseResult[ChunkValues::sizeSquared];
			
			for (int i = offsetStart; i < offsetsEnd; ++i) {
				const WorldXZPosition &fullChunkOffset = newOffsets[i]; // Get the full chunk offset
				// Calculate the noise values for terrain generation
				SetPerlinValues(noiseResults, fullChunkOffset * static_cast<PosType>(ChunkValues::size));
				
				// Create each chunk of the full chunk
				WorldPosition offset = { fullChunkOffset.x, PosType{}, fullChunkOffset.y };
				do {
					Chunk *newChunk = new Chunk();
					chunkArray[chunksStart++] = newChunk;
					newChunk->ConstructChunk(noiseResults, threadMap, offset);
				} while (++offset.y < static_cast<PosType>(ChunkValues::heightCount));
			}

			delete[] noiseResults; // Ensure noise array is deleted
		}, thread, start, threadIndex);
	}

	// Wait for all threads to finish and set the offset pointers of each chunk
	for (int threadInd = 0; threadInd < game.numThreads; ++threadInd) {
		game.genThreads[threadInd].join(); // Wait for the thread to finish
		
		// Merge the thread queue map with the main one
		Chunk::BlockQueueMap &threadMap = threadMaps[threadInd];
		for (const auto &it : threadMap) {
			Chunk::BlockQueueVector &main = m_blockQueue[it.first];
			const Chunk::BlockQueueVector threadVec = it.second;
			main.insert(main.end(), threadVec.begin(), threadVec.end());
		}
	}

	for (int i = 0; i < chunkArrayLen; ++i) {
		// Offsets are in a specific order so they can be calculated instead of stored
		const WorldXZPosition &fullOffset = newOffsets[i / ChunkValues::heightCount];
		const WorldPosition offset = { fullOffset.x, static_cast<PosType>(i % ChunkValues::heightCount), fullOffset.y };

		Chunk *chunk = chunkArray[i];
		// Add surrounding chunks to 'affected' map
		affectedChunks[offset] = chunk; // Add current chunk
		chunk->offset = &allchunks.insert({ offset, chunk }).first->first; // Set offset pointer
		AddSurrounding(affectedChunks, nearbyData, offset, false); // Add surrounding chunks to affected map
	}

	// Offsets and chunk array no longer needed - use affected map
	delete[] chunkArray;
	delete[] newOffsets;

	// Apply any block queue present
	std::vector<Chunk::BlockQueuePair> vecVals;
	std::transform(m_blockQueue.begin(), m_blockQueue.end(), std::back_inserter(vecVals), [&](Chunk::BlockQueuePair &p) { return p; });
	for (const auto &pair : vecVals) ApplyQueue(pair.second, pair.first, false);

	// Calculate chunks in the 'affected' map in parallel
	const int affectedSize = static_cast<int>(affectedChunks.size()), numChunksEach = affectedSize / static_cast<std::size_t>(game.numThreads);
	int numChunksLast = affectedSize - static_cast<std::size_t>(numChunksEach * game.numThreads);

	// Put map values into array for non-linear access
	int chunkCalcArrayIndex = 0;
	Chunk **chunkCalcArray = new Chunk*[affectedSize];
	for (const auto &it : affectedChunks) chunkCalcArray[chunkCalcArrayIndex++] = it.second;

	// Split chunk calculation amongst multiple threads
	for (int thread = 0, threadStartIndex = 0; thread < game.numThreads; ++thread) {
		const int arrayStart = threadStartIndex; // Starting index
		threadStartIndex += numChunksEach + (numChunksLast-- > 0 ? 1 : 0); // Spread the excess chunks across threads
		
		// Calculate in parallel
		game.genThreads[thread] = std::thread([&](int start, int end) {
			std::uint32_t *quadData = new std::uint32_t[ChunkValues::blocksAmount];
			for (int i = start; i < end; ++i) chunkCalcArray[i]->CalculateTerrainData(allchunks, quadData);
			delete[] quadData;
		}, arrayStart, threadStartIndex);
	}

	// Wait for all threads to finish and delete them
	for (int t = 0; t < game.numThreads; ++t) game.genThreads[t].join();

	delete[] chunkCalcArray; // Clear chunk array

	// Remove block queues in far chunks (could keep, but would stay forever even if the player moved far away)
	for (auto it = m_blockQueue.cbegin(); it != m_blockQueue.cend();) { 
		if (PlayerChunkDistance(it->first) >= static_cast<PosType>(static_cast<int>(chunkRenderDistance) + 2)) m_blockQueue.erase(it++); else ++it;
	}
	
	UpdateWorldBuffers(); // Update world buffers to use new chunk data
}

void World::UpdateWorldBuffers() noexcept
{
	// Counters for faces and face data pointer
	int faceDataPointersCount = 0;
	squaresCount = std::uint32_t{};

	// Loop through all of the chunks and each of their 6 face data to determine how much memory is needed
	// overall and accumulate all the valid pointer data into the array, as well as preparing to delete any far away chunks
	for (const auto &it : allchunks) {
		// Loop through all of the chunk's face data
		for (Chunk::FaceAxisData &faceData : it.second->chunkFaceData) {
			const std::uint32_t totalFaces = faceData.TotalFaces<std::uint32_t>();
			if (!totalFaces) continue; // This chunk has no faces, so no need to do anything
			faceDataPointers[faceDataPointersCount++] = &faceData;
			squaresCount += totalFaces;
		}
	}

	// Bind world vertex array and instanced buffer
	glBindVertexArray(m_worldVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_worldInstancedVBO);

	// World data buffer (on initial update, the world data is uninitialized so do not access)
	std::uint32_t *activeWorldData = nullptr;
	if (canMap) activeWorldData = static_cast<std::uint32_t*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));

	// Create new int array for all chunk data and index value
	std::uint32_t *newWorldData = new std::uint32_t[squaresCount];
	std::uint32_t newIndex{};

	// If a chunk face's instance data does not exist, it has been deleted and buffered before so the data can instead
	// be retrieved from the current world buffer (glMapBuffer). Otherwise, buffer the new data into the world array. 
	for (int i = 0; i < faceDataPointersCount; ++i) {
		Chunk::FaceAxisData *faceData = faceDataPointers[i];
		const std::uint32_t totalFaces = faceData->TotalFaces<std::uint32_t>();
		const std::size_t totalFacesBytes = sizeof(std::uint32_t) * static_cast<std::size_t>(totalFaces);

		if (!faceData->instancesData) std::memcpy(newWorldData + newIndex, activeWorldData + faceData->dataIndex, totalFacesBytes);
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
	game.perfs.renderSort.Start();

	// Offset value data
	ShaderChunkFace offsetData;
	m_indirectCalls = 0;

	// Counter variables
	std::size_t translucentChunksCount{};
	renderSquaresCount = std::uint32_t{};
	renderChunksCount = renderSquaresCount;

	// Radius of a sphere that would encapsulate an entire chunk - calculated
	// by half the distance from two opposing corners of a cube (side length = chunk size)
	// (Using simple radius check for frustum culling to save on performance)
	const double dblSize = static_cast<double>(ChunkValues::size);
	const double chunkSphereRadius = Math::pythagoras(Math::pythagoras(dblSize, dblSize), dblSize) * 0.5;
	const glm::dvec3 centerOffset = glm::dvec3(dblSize * 0.5); // Offset from chunk corner to center

	// After storing the normal face data for every chunk, loop through the individual chunk faces
	// that have translucent faces, giving the offset and chunk data for each
	for (const auto &it : allchunks) {
		Chunk *chunk = it.second;
		if (!chunk->chunkBlocks) continue; // Ignore 'air' (empty) chunks

		const WorldPosition &offset = it.first;

		// Use frustum culling to determine if the chunk is on-screen
		const glm::dvec3 corner = offset * static_cast<PosType>(ChunkValues::size); // Get chunk corner
		if (!player.frustum.SphereInFrustum(corner + centerOffset, chunkSphereRadius)) continue;
		++renderChunksCount;
		
		// Set offset data for shader
		offsetData.worldPositionX = corner.x;
		offsetData.worldPositionZ = corner.z;

		for (std::uint32_t faceIndex{}; faceIndex < static_cast<std::uint32_t>(6); ++faceIndex) {
			const Chunk::FaceAxisData &faceData = chunk->chunkFaceData[faceIndex];
			if (!faceData.TotalFaces<std::uint32_t>()) continue; // No faces present

			// Store world Y position and face index in a single variable (last 3 bits = index, rest are Y position)
			offsetData.faceIndexAndY = static_cast<std::uint32_t>(corner.y) + (faceIndex << static_cast<std::uint32_t>(29));
			
			// Add to vector if transparency is present
			if (faceData.translucentFaceCount) {
				ChunkTranslucentData &translucentData = translucentChunks[translucentChunksCount++];
				translucentData.chunk = chunk;
				translucentData.offsetData = offsetData;
			}

			// Check if it would even be possible to see this (opaque) face of the chunk
			// e.g. you can't see forward faces when looking north at a chunk
			
			switch (faceIndex) {
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

			// Set indirect and offset data at the same indexes in both buffers
			worldIndirectData[m_indirectCalls] = { 4u, faceData.faceCount, 0u, faceData.dataIndex };
			worldOffsetData[m_indirectCalls++] = offsetData; // Advance to next indirect call
			renderSquaresCount += faceData.TotalFaces<std::uint32_t>();
		}
	}

	// Loop through all of the sorted chunk faces with translucent faces and save the specific data
	for (std::size_t i{}; i < translucentChunksCount; ++i) {
		// Get chunk face data
		const ChunkTranslucentData &data = translucentChunks[i];
		const Chunk::FaceAxisData &faceData = data.chunk->chunkFaceData[data.offsetData.faceIndexAndY >> static_cast<std::uint32_t>(29u)];

		// Create indirect command with offset using the translucent face data
		worldIndirectData[m_indirectCalls] = { 4u, faceData.translucentFaceCount, 0u, faceData.dataIndex + faceData.faceCount };
		worldOffsetData[m_indirectCalls++] = data.offsetData;
	}

	// Bind world vertex array and SSBO to edit correct buffers
	glBindVertexArray(m_worldVAO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_worldSSBO);

	// Update SSBO and indirect buffer with their respective data and sizes
	glBufferSubData(GL_DRAW_INDIRECT_BUFFER, GLintptr{}, static_cast<GLsizeiptr>(sizeof(IndirectDrawCommand) * static_cast<std::size_t>(m_indirectCalls)), worldIndirectData);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, GLintptr{}, static_cast<GLsizeiptr>(sizeof(ShaderChunkFace) * static_cast<std::size_t>(m_indirectCalls)), worldOffsetData);

	game.perfs.renderSort.End();
}

int World::GetNearbyChunks(const WorldPosition &offset, NearbyChunkData *nearbyData, bool includeY) const noexcept
{
	int found = 0, dirIndex = -1;

	for (const WorldPosition &dir : game.constants.worldDirections) {
		++dirIndex; // Advance to next 'direction'
		if (!includeY && (dir.y != 0)) continue; // Ignore 'Y' offsets when choosing not to include them

		// Find chunk and check if it exists
		const auto &foundChunk = allchunks.find(offset + dir);
		if (foundChunk == allchunks.end()) continue;

		// Add to nearby data
		nearbyData[found++] = { foundChunk->second, static_cast<WorldDirection>(dirIndex) };
	}

	return found; // Only loop thru valid parts of nearby data array
}

int World::GetIndirectCalls() const noexcept { return static_cast<int>(m_indirectCalls); }

int World::GetNumChunks(bool includeHeight) const noexcept
{
	const int crd = static_cast<int>(chunkRenderDistance);
	const int patternChunks = (2 * crd * crd) + (2 * crd) + 1;
	return patternChunks * (includeHeight ? ChunkValues::heightCount : 1);
}

World::~World() noexcept
{
	// Delete all chunks
	for (const auto &it : allchunks) delete it.second;

	// Delete created buffer objects
	const GLuint deleteBuffers[] = { 
		m_worldSSBO, 
		m_worldIBO, 
		m_worldInstancedVBO, 
		m_worldPlaneVBO,
	};
	glDeleteBuffers(static_cast<GLsizei>(Math::size(deleteBuffers)), deleteBuffers);

	// Delete world VAO
	glDeleteVertexArrays(1, &m_worldVAO);

	// Delete stored arrays
	delete[] faceDataPointers;
	delete[] surroundingOffsets;
	delete[] translucentChunks;
	delete[] worldIndirectData;
	delete[] worldOffsetData;
	delete[] threadMaps;
}
