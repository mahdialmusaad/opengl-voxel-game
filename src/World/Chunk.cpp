#include "Chunk.hpp"

void Chunk::ConstructChunk(const WorldPerlin::NoiseResult *perlinResults, BlockQueueMap &blockQueue, WorldPosition offset) noexcept 
{
	this->offset = &offset; // Set temporary offset pointer (actual is set after full chunks finish generating)
	const int worldCornerY = static_cast<int>(offset.y) * ChunkValues::size;

	// The chunk has not been created yet so initially use a full array
	// and use an air chunk instead if no blocks are present after creation
	AllocateChunkBlocks();

	// Counter of how many total blocks are air
	std::int32_t airCounter{};

	for (int z = 0, perlin = 0; z < ChunkValues::size; ++z) {
		for (int x = 0; x < ChunkValues::size; ++x) {
			// Chunks with the same XZ offset will use the same positions for the perlin noise calculation
			// and therefore will get the same result each time, so it's much better to reuse it
			const WorldPerlin::NoiseResult &noise = perlinResults[perlin++];
			// Calculated height at this XZ position using the precalculated noise value
			const int terrainHeight = static_cast<int>(noise.landHeight);

			// Loop through Y axis
			for (int y = 0; y < ChunkValues::size; ++y) {
				const int worldY = worldCornerY + y;
				ObjectID finalBlock = ObjectID::Grass;

				if (worldY == terrainHeight) { // Block is at the surface
					// Blocks nearer to the water surface are sand whilst the rest are dirt, 
					// else grass (default block) is placed on the surface
					if (worldY <= ChunkValues::waterMaxHeight) {
						bool closeToWater = terrainHeight - worldY < 2;
						finalBlock = closeToWater ? ObjectID::Sand : ObjectID::Dirt;
					} else AttemptGenerateTree(blockQueue, x, y, z, noise, ObjectID::Log, ObjectID::Leaves);
				}
				else if (worldY < terrainHeight) { // Block is under surface
					// Blocks slightly under surface are dirt
					if (terrainHeight - worldY <= ChunkValues::baseDirtHeight) finalBlock = ObjectID::Dirt;
					// Blocks far enough underground are stone
					else finalBlock = ObjectID::Stone;
				}
				else { // Block is above surface
					// Fill low surfaces with water
					if (worldY < ChunkValues::waterMaxHeight) finalBlock = ObjectID::Water;
					// Rest of the blocks are air as this is above the 'height', add to air counter
					else { airCounter += ChunkValues::size - y; break; }
				}

				chunkBlocks->blocks[x][y][z] = finalBlock; // Set the block at the corresponding position
			}
		}
	}

	// If the chunk is just air, there is no need to store every single block
	if (airCounter == ChunkValues::blocksAmount) {
		delete chunkBlocks;
		chunkBlocks = nullptr;
	}
}

void Chunk::AttemptGenerateTree(BlockQueueMap &treeBlocksQueue, int x, int y, int z, const WorldPerlin::NoiseResult &noise, ObjectID logID, ObjectID leavesID) noexcept
{
	if (!NoiseValueRand(noise, ChunkValues::treeSpawnChance)) return;

	const int treeHeight = static_cast<int>(noise.flatness * 3.0f) + 5;
	const WorldPosition above = *offset + game.constants.worldDirections[WldDir_Up];
	const std::uint8_t leavesStrength = ChunkValues::GetBlockData(leavesID).strength;
	const int csz = ChunkValues::size;

	for (int logY = y + 1, logTop = logY + treeHeight; logY < logTop; ++logY) {
		if (logY >= ChunkValues::size) AddBlockQueue(treeBlocksQueue, above, { {x, Math::loop(logY, 0, csz), z}, logID, true });
		else chunkBlocks->blocks[x][logY][z] = logID;
	}

	// Use bitwise check to quickly determine if leaf position is inside chunk
	const int allBitsExceptMax = ~ChunkValues::sizeLess;

	// Leaf placing function (place in queue if the leaf is outside local chunk)
	// Natural placement, so the leaf could be overriden by a 'stronger' block
	const auto PossibleSpawnLeaf = [&](int leavesX, int leavesY, int leavesZ) {
		if ((leavesX | leavesY | leavesZ) & allBitsExceptMax) {
			glm::dvec3 leafPos = { leavesX, leavesY, leavesZ };
			const WorldPosition outerOffset = *offset + ChunkValues::WorldToOffset(leafPos);
			AddBlockQueue(treeBlocksQueue, outerOffset, { { Math::loop(leavesX, 0, csz), Math::loop(leavesY, 0, csz), Math::loop(leavesZ, 0, csz) }, leavesID, true });
		} else if (ChunkValues::GetBlockData(chunkBlocks->blocks[leavesX][leavesY][leavesZ]).strength <= leavesStrength) chunkBlocks->blocks[leavesX][leavesY][leavesZ] = leavesID;
	};
	
	// Place main leaves around log
	bool isTopLeaf = true;
	int leafChance = 4;

	for (int endY = y + treeHeight - 3, leavesY = endY + 2; leavesY >= endY; --leavesY) {
		for (int leavesZ = z - 2, endZ = leavesZ + 4; leavesZ <= endZ; ++leavesZ) {
			const bool midZ = leavesZ == z;
			const bool edgeZ = leavesZ == z - 2 || leavesZ == endZ;
			for (int leavesX = x - 2, endX = leavesX + 4; leavesX <= endX; ++leavesX) {
				const bool midX = leavesX == x;
				const bool bothEdges = edgeZ && (leavesX == x - 2 || leavesX == endX);

				if (midX && midZ) continue; // Don't attempt to place where logs are
				if (isTopLeaf && bothEdges && NoiseValueRand(noise, leafChance)) continue; // Top corner leaves are not guaranteed

				PossibleSpawnLeaf(leavesX, leavesY, leavesZ);
				leafChance += 20; // Decrease chance for other corner leaves
			}
		}

		isTopLeaf = false;
	}

	// Arrangement of top leaves
	const int leavesSquares[17][3] = { 
		{-1, 0, -1}, {-1, 0, 0}, {-1, 0, 1}, {0, 0, -1}, {0, 0, 1}, {1, 0, -1}, {1, 0, 0}, {1, 0, 1},
		{-1, 1, 0}, {0, 1, -1}, {0, 1, 1}, {1, 1, 0}, {0, 1, 0}
	};

	for (const int (&leafPosition)[3] : leavesSquares) PossibleSpawnLeaf(x + leafPosition[0], y + treeHeight + leafPosition[1], z + leafPosition[2]);
}

void Chunk::AddBlockQueue(BlockQueueMap &map, const WorldPosition &offset, const BlockQueue &queue)
{
	// Each thread is given an individual map, so no need for a locking mechanism.
	// Helper function to just add block queue to an offset in the map
	map[offset].emplace_back(queue);
}

bool Chunk::NoiseValueRand(const WorldPerlin::NoiseResult &noise, int oneInX) noexcept
{
	// Determine chance based on given values

	// Large integer value for bitwise manipulation
	std::int64_t mult = static_cast<std::int64_t>((noise.flatness * 8278555403.357f) - ((noise.temperature - noise.humidity) * 347894783.546f));

	// XOR and shift to get a hash of the noise value
	mult ^= mult << 13;
	mult ^= mult >> 17;
	mult ^= mult << 5;
	return !(mult % oneInX);
}

void Chunk::CalculateTerrainData(WorldMapDef &chunksMap) noexcept
{
	// To improve performance, the quad data is defined beforehand during chunk generation.
	// However, the chunk faces are calculated in other scenarios (e.g. breaking and placing blocks) where this is not done.
	std::uint32_t *quadData = new std::uint32_t[ChunkValues::blocksAmount];
	CalculateTerrainData(chunksMap, quadData);
	delete[] quadData;
}

void Chunk::CalculateTerrainData(WorldMapDef &chunksMap, std::uint32_t *quadData) noexcept
{
	if (!chunkBlocks) return; // Don't calculate air chunks

	// Store nearby chunks in an array for easier access (last index is current chunk)
	const ChunkValues::BlockArray *localNearby[7] {};
	localNearby[6] = chunkBlocks; // Last one points to this chunk

	for (int i = 0; i < 6; ++i) {
		const auto &foundChunkIt = chunksMap.find(*offset + game.constants.worldDirections[i]); // Look for a chunk in each direction
		Chunk *foundChunk = foundChunkIt == chunksMap.end() ? nullptr : foundChunkIt->second; // Pointer to chunk or nullptr if none exists
		if (foundChunk && foundChunk->chunkBlocks) localNearby[i] = foundChunk->chunkBlocks; // Add blocks struct to nearby pointers if the chunk and its blocks exist
		else localNearby[i] = &ChunkValues::emptyChunk;
	}

	// Loop through each array in the given face data array 
	std::size_t lookupIndex{};
	for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
		FaceAxisData &faceData = chunkFaceData[faceIndex];

		// Reset any previous data
		faceData.translucentFaceCount = std::uint16_t{};
		faceData.faceCount = std::uint16_t{};

		// Loop through all the chunk's blocks for each face direction
		for (int x = 0; x < ChunkValues::size; ++x) {
			const ObjectID (&outerBlockArray)[ChunkValues::size][ChunkValues::size] = chunkBlocks->blocks[x];
			const std::int32_t xIndex = static_cast<std::int32_t>(x);
			for (int y = 0; y < ChunkValues::size; ++y) {
				const ObjectID (&innerBlockArray)[ChunkValues::size] = outerBlockArray[y];
				const std::int32_t yIndex = static_cast<std::int32_t>(y << ChunkValues::sizeBits);
				for (int z = 0; z < ChunkValues::size; ++z) {
					const WorldBlockData &currentBlock = ChunkValues::GetBlockData(innerBlockArray[z]); // Get properties of the current block
					const ChunkLookupData &nearbyData = chunkLookupData[lookupIndex++]; // Get precalculated results for the next block's position and face index
					// Get the data of the block next to the current face, checking the correct chunk
					const WorldBlockData &nextBlock =  ChunkValues::GetBlockData(localNearby[nearbyData.index]->at(nearbyData.pos));
					
					// Check if the face is not obscured by the block found to be next to it
					if (!currentBlock.notObscuredBy(currentBlock, nextBlock)) continue;
					
					// Compress the position and texture data into one integer
					// Layout: TTTT TTTT TTTT TTTT TZZZ ZZYY YYYX XXXX
					const std::uint32_t newData = static_cast<std::uint32_t>(
					    xIndex + yIndex + static_cast<int32_t>(z << (ChunkValues::sizeBits * 2)) + // Position in chunk
					    static_cast<std::int32_t>(static_cast<int>(currentBlock.textures[faceIndex]) << (ChunkValues::sizeBits * 3)) // Texture
					);

					// Blocks with transparency need to be rendered last for them to be rendered
					// correctly on top of existing terrain, so they can be placed starting from
					// the end of the data array instead to be seperated from the opaque blocks

					// Set the data in reverse order, starting from the end of the array 
					// (pre-increment to avoid writing to out of bounds the first time)
					// If it is a normal face however, just add to the array normally.
					quadData[currentBlock.hasTransparency ? ChunkValues::blocksAmount - ++faceData.translucentFaceCount : faceData.faceCount++] = newData;
				}
			}
		}

		// Compress the array as there is most likely a gap between the normal face data
		// and the translucent faces as the total face count is known now
		const std::size_t totalFaces = faceData.TotalFaces<std::size_t>();
		if (!totalFaces) continue; // No need for compression if there is no data in the first place

		// Allocate the exact amount of data needed to store all the faces
		faceData.instancesData = new std::uint32_t[totalFaces];

		// Copy the opaque and translucent sections of the face data to be 
		// next to each other in the new compressed array

		// Opaque face data
		std::memcpy(faceData.instancesData, quadData, sizeof(std::uint32_t) * faceData.faceCount);
		
		// Translucent face data
		std::memcpy(
			faceData.instancesData + faceData.faceCount,
			quadData + (ChunkValues::blocksAmount - faceData.translucentFaceCount),
			sizeof(std::uint32_t) * faceData.translucentFaceCount
		);

		// The data now is stored as such (assuming no bugs):
		// [(opaque data)(translucent data)] <--- No gaps, exact size is allocated
	}
}

void Chunk::AllocateChunkBlocks() noexcept
{
	// Allocate memory for chunk blocks array if it does not already exist and set it to all 0's
	if (chunkBlocks) return;
	chunkBlocks = new ChunkValues::BlockArray;
	std::memset(chunkBlocks->blocks, static_cast<int>(ObjectID::Air), sizeof(ChunkValues::BlockArray));
}

Chunk::~Chunk()
{
	for (FaceAxisData &fd : chunkFaceData) if (fd.instancesData) delete[] fd.instancesData; // Remove instance face data (if any)
	if (chunkBlocks) delete chunkBlocks; // Delete chunk block data
}
