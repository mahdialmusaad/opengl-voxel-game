#include "Chunk.hpp"

Chunk::Chunk(WorldPos offset) noexcept : 
	positionMagnitude(sqrt(static_cast<double>((offset.x * offset.x) + (offset.y * offset.y) + (offset.z * offset.z)))) 
{}

void Chunk::ConstructChunk(const WorldPerlin::NoiseResult *perlinResults, BlockQueueMap &blockQueue) noexcept 
{
	// Y position in the world of the chunk (corner)
	const int worldYCorner = static_cast<int>(offset->y) * ChunkSettings::CHUNK_SIZE;

	// The chunk has not been created yet so initially use a full array
	// and use an air chunk instead if no blocks are present after creation
	AllocateChunkBlocks();

	// Counter of how many total blocks are air
	std::int32_t airCounter = 0;

	for (int z = 0, perlin = 0; z < ChunkSettings::CHUNK_SIZE; ++z) {
		for (int x = 0; x < ChunkSettings::CHUNK_SIZE; ++x) {
			// Chunks with the same XZ offset (different Y) will use the same XZ values for the perlin noise calculation
			// and so will get the same result each time, so it's much better to precalculate the full chunk values
			const WorldPerlin::NoiseResult &positionNoise = perlinResults[perlin++];
			// Calculated height at this XZ position using the precalculated noise value
			const int terrainHeight = static_cast<int>(positionNoise.continentalnessHeight);

			// Loop through Y axis
			for (int y = 0; y < ChunkSettings::CHUNK_SIZE; ++y) {
				const int worldY = y + worldYCorner; // Y position in world coordinates
				ObjectID finalBlock = ObjectID::Grass;

				if (worldY == terrainHeight) { // Block is at the surface
					// Blocks nearer to the water surface are sand whilst the rest are dirt, 
					// else grass (default block) is placed on the surface
					if (worldY <= ChunkSettings::CHUNK_WATER_HEIGHT) {
						bool closeToWater = terrainHeight - worldY < 2;
						finalBlock = closeToWater ? ObjectID::Sand : ObjectID::Dirt;
					} else AttemptGenerateTree(blockQueue, x, y, z, positionNoise, ObjectID::Log, ObjectID::Leaves);
				}
				else if (worldY < terrainHeight) { // Block is under surface
					// Blocks slightly under surface are dirt
					if (terrainHeight - worldY <= ChunkSettings::CHUNK_BASE_DIRT_HEIGHT) finalBlock = ObjectID::Dirt;
					// Blocks far enough underground are stone
					else finalBlock = ObjectID::Stone;
				}
				else { // Block is above surface
					// Fill low surfaces with water
					if (worldY < ChunkSettings::CHUNK_WATER_HEIGHT) finalBlock = ObjectID::Water;
					// Rest of the blocks are air as this is above the 'height', add to air counter
					else { airCounter += ChunkSettings::CHUNK_SIZE - y; break; }
				}

				chunkBlocks->blocks[x][y][z] = finalBlock; // Set the block at the corresponding position
			}
		}
	}

	// If the chunk is just air, there is no need to store every single block
	if (airCounter == ChunkSettings::CHUNK_BLOCKS_AMOUNT) {
		delete chunkBlocks;
		chunkBlocks = nullptr;
	}
}

void Chunk::AttemptGenerateTree(BlockQueueMap &treeBlocksQueue, int x, int y, int z, const WorldPerlin::NoiseResult &noise, ObjectID logID, ObjectID leavesID) noexcept
{
	if (!NoiseValueRand(noise, ChunkSettings::TREE_GEN_CHANCE)) return;

	const int treeHeight = static_cast<int>(noise.flatness * 3.0f) + 5;
	const WorldPos above = *offset + game.constants.worldDirections[WldDir_Up];
	const std::uint8_t leavesStrength = ChunkSettings::GetBlockData(leavesID).strength;

	for (int logY = y + 1, logTop = logY + treeHeight; logY < logTop; ++logY) {
		if (logY >= ChunkSettings::CHUNK_SIZE) treeBlocksQueue[above].emplace_back(BlockQueue(x, Math::loopAround(logY, 0, 32), z, logID, true));
		else chunkBlocks->blocks[x][logY][z] = logID;
	}

	// Use bitwise check to quickly determine if leaf position is inside chunk
	const int allBitsExceptMax = ~ChunkSettings::CHUNK_SIZE_M1;

	// Leaf placing lambda (place in queue if the leaf is outside local chunk)
	const auto PossibleSpawnLeaf = [&](int leavesX, int leavesY, int leavesZ) {
		if ((leavesX | leavesY | leavesZ) & allBitsExceptMax) {
			glm::dvec3 leafPos = { leavesX, leavesY, leavesZ };
			const WorldPos newChunkOffset = *offset + ChunkSettings::WorldToOffset(leafPos);
			treeBlocksQueue[newChunkOffset].emplace_back(BlockQueue(Math::loopAround(leavesX, 0, 32), Math::loopAround(leavesY, 0, 32), Math::loopAround(leavesZ, 0, 32), leavesID, true));
		} else if (ChunkSettings::GetBlockData(chunkBlocks->blocks[leavesX][leavesY][leavesZ]).strength <= leavesStrength) chunkBlocks->blocks[leavesX][leavesY][leavesZ] = leavesID;
	};
	
	// Place main leaves around log
	bool isTopLeaf = true;
	for (int endY = y + treeHeight - 3, leavesY = endY + 2; leavesY >= endY; --leavesY) {
		for (int leavesZ = z - 2, endZ = leavesZ + 4; leavesZ <= endZ; ++leavesZ) {
			const bool midZ = leavesZ == z;
			const bool edgeZ = leavesZ == z - 2 || leavesZ == endZ;
			for (int leavesX = x - 2, endX = leavesX + 4; leavesX <= endX; ++leavesX) {
				const bool midX = leavesX == x;
				const bool bothEdges = edgeZ && (leavesX == x - 2 || leavesX == endX);

				if (midX && midZ) continue; // Don't attempt to place where logs are
				if (isTopLeaf && bothEdges && NoiseValueRand(noise.flatness * static_cast<float>(leavesX + leavesZ * y), 4)) continue; // Top corner leaves are not guaranteed

				PossibleSpawnLeaf(leavesX, leavesY, leavesZ);
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

bool Chunk::NoiseValueRand(const WorldPerlin::NoiseResult &noise, int oneInX) noexcept
{
	// Determine chance based on given values

	// Large integer value for bitwise manipulation
	std::int64_t mult = static_cast<int64_t>((noise.flatness * 8278555403.357f) - ((noise.temperature - noise.humidity) * 347894783.546f));

	// XOR and shift to get a hash of the noise value
	mult ^= mult << 13;
	mult ^= mult >> 17;
	mult ^= mult << 5;
	return !(mult % oneInX);
}

void Chunk::CalculateTerrainData(WorldMapDef &chunksMap) noexcept
{
	if (!chunkBlocks) return; // Don't calculate air chunks

	// Store nearby chunks in an array for easier access (last index is current chunk)
	const ChunkSettings::BlockArray *localNearby[7] {};
	localNearby[6] = chunkBlocks; // Last one points to this chunk

	for (int i = 0; i < 6; ++i) {
		const auto &foundChunkIt = chunksMap.find(*offset + game.constants.worldDirections[i]); // Look for a chunk in each direction
		Chunk *foundChunk = foundChunkIt == chunksMap.end() ? nullptr : foundChunkIt->second; // Pointer to chunk or nullptr if none exists
		if (foundChunk && foundChunk->chunkBlocks) localNearby[i] = foundChunk->chunkBlocks; // Add blocks struct to nearby pointers if the chunk and its blocks exist
		else localNearby[i] = &ChunkSettings::emptyChunk;
	}

	// Create an array with the maximum size on the outside to be used per chunk face calculation
	std::uint32_t *outer = new std::uint32_t[ChunkSettings::CHUNK_BLOCKS_AMOUNT];

	// Loop through each array in the given face data array 
	std::size_t lookupIndex{};
	for (std::size_t faceIndex = 0; faceIndex < 6u; ++faceIndex) {
		FaceAxisData &faceData = chunkFaceData[faceIndex];

		// Reset any previous data
		faceData.translucentFaceCount = 0u;
		faceData.faceCount = 0u;

		// Loop through all the chunk's blocks for each face direction
		for (int x = 0; x < ChunkSettings::CHUNK_SIZE; ++x) {
			const ChunkSettings::BlockArray::OuterArrayDef &outerBlockArray = chunkBlocks->blocks[x];
			for (int y = 0; y < ChunkSettings::CHUNK_SIZE; ++y) {
				const std::int32_t yIndex = y << 5;
				const ChunkSettings::BlockArray::InnerArrayDef &innerBlockArray = outerBlockArray[y];
				for (int z = 0; z < ChunkSettings::CHUNK_SIZE; ++z) {
					const WorldBlockData &currentBlock = ChunkSettings::GetBlockData(innerBlockArray[z]); // Get properties of the current block
					const ChunkLookupData &nearbyData = chunkLookupData[lookupIndex++]; // Get precalculated results for the next block's position and face index
					// Get the data of the block next to the current face, checking the correct chunk
					const WorldBlockData &nextBlock =  ChunkSettings::GetBlockData(localNearby[nearbyData.index]->at(nearbyData.pos));
					
					// Check if the face is not obscured by the block found to be next to it
					if (currentBlock.notObscuredBy(currentBlock, nextBlock)) {
						// Compress the position and texture data into one integer
						// Layout: TTTT TTTT TTTT TTTT TZZZ ZZYY YYYX XXXX
						const std::int32_t positionIndex = yIndex + static_cast<int32_t>(z << 10) + static_cast<std::int32_t>(x);
						const std::int32_t textureData = static_cast<std::int32_t>(currentBlock.textures[faceIndex]) << 15;
						const std::uint32_t newData = static_cast<std::uint32_t>(positionIndex + textureData);

						// Blocks with transparency need to be rendered last for them to be rendered
						// correctly on top of existing terrain, so they can be placed starting from
						// the end of the data array instead to be seperated from the opaque blocks

						// Set the data in reverse order, starting from the end of the array 
						// (pre-increment to avoid writing to out of bounds the first time)
						if (currentBlock.hasTransparency) outer[ChunkSettings::CHUNK_BLOCKS_AMOUNT - ++faceData.translucentFaceCount] = newData;
						else outer[faceData.faceCount++] = newData; // Append new data to the current chunk face data and increment face counter
					}
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
		std::memcpy(faceData.instancesData, outer, sizeof(std::uint32_t) * faceData.faceCount);
		
		// Translucent face data
		std::memcpy(
			faceData.instancesData + faceData.faceCount,
			outer + (ChunkSettings::CHUNK_BLOCKS_AMOUNT - faceData.translucentFaceCount),
			sizeof(std::uint32_t) * faceData.translucentFaceCount
		);

		// The data now is stored as such (assuming no bugs):
		// [(opaque data)(translucent data)] <--- No gaps, exact size is allocated
	}

	// Ensure outer data array is cleaned up
	delete[] outer;
}

void Chunk::AllocateChunkBlocks() noexcept
{
	// Allocate memory for chunk blocks array if it does not already exist
	if (chunkBlocks) return;
	chunkBlocks = new ChunkSettings::BlockArray;
}

Chunk::~Chunk()
{
	for (FaceAxisData &fd : chunkFaceData) if (fd.instancesData) delete[] fd.instancesData; // Remove instance face data (if any)
	if (chunkBlocks) delete chunkBlocks; // Delete chunk block data
}
