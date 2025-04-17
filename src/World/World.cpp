#include "World.hpp"

World::World(PlayerObject& player) noexcept : player(player)
{
	TextFormat::log("World enter");
	
	// Create VAO for the world buffers to store buffer settings
	m_worldVAO = OGL::CreateVAO16();
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);
	glEnableVertexAttribArray(2u);
	glEnableVertexAttribArray(3u);
	glVertexAttribDivisor(0u, 1u);

	// World data buffer (allocation happens on world update)
	m_worldVBO = OGL::CreateBuffer8(GL_ARRAY_BUFFER);
	glVertexAttribIPointer(0u, 1, GL_UNSIGNED_INT, sizeof(uint32_t), nullptr);

	// Very important definition of a plane/quad for block rendering
	constexpr int32_t planeVertices[] = {
	//	  ivec4 0		  ivec4 1		  ivec4 2
		1, 1, 0, 1,		0, 1, 0, 1,		0, 0, 0, 0,	 // Vertex 1
		1, 0, 0, 1,		0, 0, 0, 1,		0, 1, 0, 0,	 // Vertex 2
		0, 1, 0, 1,		1, 1, 0, 1,		1, 0, 0, 0,	 // Vertex 3
		0, 0, 0, 1,		1, 0, 0, 1,		1, 1, 0, 0,	 // Vertex 4
	//	x  z  0  1      y  z  0  1      y  w  0  0                 
	};

	// Instanced VBO that contains plane data, 12 floats per vertex
	OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribIPointer(1u, 4, GL_INT, sizeof(int32_t[12]), nullptr);
	glVertexAttribIPointer(2u, 4, GL_INT, sizeof(int32_t[12]), reinterpret_cast<const void*>(sizeof(int32_t[4])));
	glVertexAttribIPointer(3u, 4, GL_INT, sizeof(int32_t[12]), reinterpret_cast<const void*>(sizeof(int32_t[8])));
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, 0);

	// Shader storage buffer object to store chunk positions for each chunk face (bound in location = 0)
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0u, OGL::CreateBuffer(GL_SHADER_STORAGE_BUFFER));
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_STORAGE_BIT);

	// Indirect buffer that contains index data for the actual world data
	OGL::CreateBuffer(GL_DRAW_INDIRECT_BUFFER);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_STORAGE_BIT);

	TextFormat::log("World settings setup");

	// Create noise splines
	constexpr int numSplines = WorldPerlin::NoiseSpline::numSplines;
	WorldPerlin::NoiseSpline noiseSplines[numSplines] {};
	// constexpr WorldPerlin::NoiseSpline::Spline splineObjects[WorldNoise::numNoises][numSplines] = {
	// 	{ {0.0f, 0.0f}, {0.1f, 10.0f}, {0.2f, 20.0f}, {0.3f, 30.0f}, {0.4f, 40.0f}, {0.5f, 50.0f}, {0.6f, 60.0f}, {0.7f, 70.0f}, {0.8f, 80.0f}, {0.9f, 90.0f} }, // Continentalness
	// 	{ {0.0f, 0.0f}, {0.1f, 10.0f}, {0.2f, 20.0f}, {0.3f, 30.0f}, {0.4f, 40.0f}, {0.5f, 50.0f}, {0.6f, 60.0f}, {0.7f, 70.0f}, {0.8f, 80.0f}, {0.9f, 90.0f} }, // Flatness
	// 	{ {0.0f, 0.0f}, {0.1f, 10.0f}, {0.2f, 20.0f}, {0.3f, 30.0f}, {0.4f, 40.0f}, {0.5f, 50.0f}, {0.6f, 60.0f}, {0.7f, 70.0f}, {0.8f, 80.0f}, {0.9f, 90.0f} }, // Depth
	// 	{ {0.0f, 0.0f}, {0.1f, 10.0f}, {0.2f, 20.0f}, {0.3f, 30.0f}, {0.4f, 40.0f}, {0.5f, 50.0f}, {0.6f, 60.0f}, {0.7f, 70.0f}, {0.8f, 80.0f}, {0.9f, 90.0f} }, // Temperature
	// 	{ {0.0f, 0.0f}, {0.1f, 10.0f}, {0.2f, 20.0f}, {0.3f, 30.0f}, {0.4f, 40.0f}, {0.5f, 50.0f}, {0.6f, 60.0f}, {0.7f, 70.0f}, {0.8f, 80.0f}, {0.9f, 90.0f} }, // Humidity
	// };

	game.noiseGenerators = WorldNoise(noiseSplines);
	// World loading, apply settings from file, etc...

	TextFormat::log("World exit");
}

void World::DrawWorld() const noexcept
{
	game.shader.UseShader(Shader::ShaderID::World);
	glBindVertexArray(m_worldVAO);
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
	else if (y < zeroPosType) {
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
	y = std::clamp(y, zeroPosType, ChunkSettings::PMAX_WORLD_HEIGHT);
	ty = std::clamp(ty, zeroPosType, ChunkSettings::PMAX_WORLD_HEIGHT);

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
    return Math::abs(playerOffset.x - chunkOffset.x) +
           Math::abs(playerOffset.z - chunkOffset.z) <= static_cast<PosType>(chunkRenderDistance);
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
	WorldPos offset = { offsetXZ.x, zeroPosType, offsetXZ.y };
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

void World::TestChunkUpdate() noexcept
{
	if (!game.mainLoopActive || game.test) return;
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
            if (abs(x) + abs(z) > chunkRenderDistance) continue;
            
            const ChunkOffset newOffset = playerOffset + ChunkOffset(x, z);
            // Check if there isn't already a chunk at the selected offset
            if (chunks.find({ newOffset.x, zeroPosType, newOffset.y }) != chunks.end()) continue;
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

	// Bind world vertex array to edit correct buffers
	glBindVertexArray(m_worldVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_worldVBO);

	if (threadUpdateBuffers) {
		// Move any newly generated chunks (from thread) to the main chunk map
		for (Chunk* chunk : m_transferChunks) chunks.insert({ chunk->GetOffset(), chunk});
		m_transferChunks.clear(); // Clear new chunk pointer array
	}

	#ifndef BADCRAFT_1THREAD
	// Vectors of chunks to be removed - cannot delete while looping through map at the same time
	std::vector<Chunk*> chunksToRemove;
	#endif

	// Counters
	int32_t faceDataPointersCount = 0;
	uint32_t amountOfInts = 0u;

    // 2n^2 + 2n + 1 = amount of chunks in a 'star' pattern around the player, where n is the render distance
    // Multiply by 6 to get the total number of 'chunk faces' and by the height count to include full chunks
    const int chunkFacesTotal = 
        ((2 * chunkRenderDistance * chunkRenderDistance) + (2 * chunkRenderDistance) + 1) *
        6 * ChunkSettings::HEIGHT_COUNT;
	
	// Pointers to chunk face objects
	typedef Chunk::FaceAxisData ChunkFaceData;
	ChunkFaceData** faceDataPointers = new ChunkFaceData*[chunkFacesTotal];

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

		// Loop through all of the chunk's face data, either adding it (if non-empty) to the end vector or to the pointer array
		for (ChunkFaceData& faceData : chunk->chunkFaceData) {
			const uint32_t totalFaces = faceData.TotalFaces<uint32_t>();
			if (!totalFaces) continue; // Empty, ignore
			faceDataPointers[faceDataPointersCount++] = &faceData;
			amountOfInts += totalFaces;
		}
	}

	#ifndef BADCRAFT_1THREAD
	// Remove all chunks that are further than current render distance
	for (Chunk* chunk : chunksToRemove) RemoveChunk(chunk);
	chunksToRemove.clear();
	#endif

	// Just clear the data if there are no chunks present
	if (!chunks.size()) {
		delete[] faceDataPointers;
		SortWorldBuffers();
		threadUpdateBuffers = false;
		return;
	}

	// World data buffer
	const uint32_t* activeWorldData = static_cast<uint32_t*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));
	// Create new int array for all chunk data and index value
	uint32_t* newWorldData = new uint32_t[amountOfInts];
	uint32_t newIndex = 0u;

	/*
		Since there is already data present in the world buffer, the old chunks' face data array
		would have been deleted (to save memory), so the actual data is found in the current world buffer data instead.

		* Deleted chunks should have their data overwritten/removed
		* Updated chunks should have their data moved to the end as the new data size will most likely change (doesn't fit/leaves empty space)
		* New chunks should be placed at the end as well
			
		Deleted chunks are identified from gaps in the 'world index'
		Updated and new chunks have new data from face calculation
		The original chunks that have not changed will not have any face data (nullptr)

		Example data layout in glMapBuffer result: [(chunk data)(old chunk data)(deleted chunk data)(chunk data)(chunk data)...]
		Example data layout after: [(chunk data)(chunk data)(chunk data)(new chunk data)...]
	*/

	// Copy the data from all valid chunk faces depending on whether it has new data or not and set new data indexes:
	// Contains new data - memcpy new instance data to new index then delete new instance data to free memory
	// Data is nullptr - memcpy from active world buffer at current set index to new world data (where the chunk's data is located)
	for (int32_t i = 0; i < faceDataPointersCount; ++i) {
		ChunkFaceData* faceData = faceDataPointers[i];
		const uint32_t totalFaces = faceData->TotalFaces<uint32_t>();
		const size_t totalFacesBytes = static_cast<size_t>(totalFaces) * sizeof(uint32_t);
		if (faceData->instancesData == nullptr) memcpy(newWorldData + newIndex, activeWorldData + faceData->dataIndex, totalFacesBytes);
		else {
			memcpy(newWorldData + newIndex, faceData->instancesData, totalFacesBytes);
			delete[] faceData->instancesData;
			faceData->instancesData = nullptr;
		}
		faceData->dataIndex = newIndex;
		newIndex += totalFaces;
	}

	// Buffer data then free memory used by world data and face data pointers
	constexpr int64_t sizeofUI32 = static_cast<int64_t>(sizeof(uint32_t));
	glBufferData(GL_ARRAY_BUFFER, static_cast<int64_t>(amountOfInts) * sizeofUI32, newWorldData, GL_STATIC_DRAW);

	delete[] newWorldData;
    delete[] faceDataPointers;

	// Ensure new chunk data is used in indirect data and SSBO
	SortWorldBuffers();
	threadUpdateBuffers = false;
}

void World::SortWorldBuffers() noexcept
{
	// Bind world vertex array and instance VBO
	glBindVertexArray(m_worldVAO);

	typedef glm::i64vec3 WorldPos64;

	struct ShaderChunkOffset {
		int64_t worldOffsetX;
		int64_t worldOffsetY;
		int64_t worldOffsetZ;
		int8_t faceIndex;
	};

	struct ChunkTranslucentData {
		ShaderChunkOffset offsetData;
		Chunk* chunk;
	};

	struct IndirectDrawCommand {
		uint32_t count;
		uint32_t instanceCount;
		uint32_t first;
		uint32_t baseInstance;
	};

	/*
		Position offsets and indirect arrays:
		Two is needed per chunk face as translucent objects are rendered separately
		after all the opaque faces so require the same data as well
	*/

    // Chunk total face calculation as explained in world data buffer update function
    const int chunkFacesTotal = 
        ((2 * chunkRenderDistance * chunkRenderDistance) + (2 * chunkRenderDistance) + 1) *
        6 * ChunkSettings::HEIGHT_COUNT;

    // Multiply by 2 as well due to the multiline comment above
	IndirectDrawCommand* worldIndirectData = new IndirectDrawCommand[chunkFacesTotal * 2];
	ShaderChunkOffset* worldOffsetData = new ShaderChunkOffset[chunkFacesTotal * 2];

	// Offset value data
	ShaderChunkOffset offsetData {};
	m_indirectCalls = 0;

	/*
		Faces that you can see through need to be rendered last for the 'transparency' effect
		to actually work and prevent any silliness like invisible normal faces when looking
		through blocks that have some sort of transparency

		After storing the normal face data for every chunk, loop through the individual chunk faces
		that have translucent faces, giving the offset and chunk data for each
	*/

	// All chunk face data with non-opaque faces go here for sorting later on
	ChunkTranslucentData* translucentChunks = new ChunkTranslucentData[chunkFacesTotal];
	int translucentChunksCount = 0;

	// Loop through all of the chunks and each of their normal face data
	for (const auto& [offset, chunk] : chunks) {
		// Set offset data for shader
		const WorldPos64 chunkPosition64 = static_cast<WorldPos64>(offset) * ChunkSettings::CHUNK_SIZE_I64;
		offsetData.worldOffsetX = chunkPosition64.x;
		offsetData.worldOffsetY = chunkPosition64.y;
		offsetData.worldOffsetZ = chunkPosition64.z;

		// TODO: add WORKING frustum culling

		for (offsetData.faceIndex = 0; offsetData.faceIndex < 6; ++offsetData.faceIndex) {
			const Chunk::FaceAxisData& faceData = chunk->chunkFaceData[offsetData.faceIndex];
			if (!faceData.TotalFaces<uint32_t>()) continue; // No faces present

			// Add to vector if transparency is present, go to next iteration if there are no faces at all
			if (faceData.translucentFaceCount > 0u) {
				ChunkTranslucentData& translucentData = translucentChunks[translucentChunksCount++];
				translucentData.chunk = chunk;
				translucentData.offsetData = offsetData;
			}
			
			// Check if it would even be possible to see this (opaque) face of the chunk
			// e.g. you can't see forward faces when looking north at a chunk

			//	2d example:		---------
			//					|opaque |
			//	camera -------> | shape | <---- impossible to see this face
			//					|		|
			//					---------

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

			// Assign indirect and offset data to array (same amount, so same index)
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
	for (int i = 0; i < translucentChunksCount; ++i) {
		const ChunkTranslucentData& data = translucentChunks[i];
		const Chunk::FaceAxisData& faceData = data.chunk->chunkFaceData[data.offsetData.faceIndex];
		const uint32_t translucentFaces = faceData.translucentFaceCount, translucentOffset = faceData.dataIndex + faceData.faceCount;
		// Create indirect command with offset using the translucent face data
		worldIndirectData[m_indirectCalls] = { 4u, translucentFaces, 0u, translucentOffset };
		worldOffsetData[m_indirectCalls++] = data.offsetData;
	}


	// Buffer the accumulated data
	const int64_t totalOffsetDataSize = static_cast<int64_t>(m_indirectCalls) * static_cast<int64_t>(sizeof(ShaderChunkOffset));
	glBufferData(GL_DRAW_INDIRECT_BUFFER, totalOffsetDataSize, worldIndirectData, GL_STATIC_DRAW);
	glBufferData(GL_SHADER_STORAGE_BUFFER, totalOffsetDataSize, worldOffsetData, GL_STATIC_COPY);

	// Free up memory used by arrays created with new
	delete[] translucentChunks;
	delete[] worldIndirectData;
	delete[] worldOffsetData;
}
