#include "Chunk.hpp"

Chunk::Chunk(WorldPos offset) noexcept : 
	positionMagnitude(sqrt(static_cast<double>((offset.x * offset.x) + (offset.y * offset.y) + (offset.z * offset.z)))) 
{}

Chunk::BlockQueueMap Chunk::ConstructChunk(WorldPerlin::NoiseResult *perlinResults) noexcept 
{
	// Stone block test
	// ChunkSettings::ChunkBlockValueFull* blocks = new ChunkSettings::ChunkBlockValueFull;
	// for(int x = 0; x < 32; ++x)for(int y= 0; y < 32; ++y)for(int z = 0; z < 32; ++z)blocks->chunkBlocks[x][y][z]=ObjectID::Stone;
	// chunkBlocks = blocks;

	// Y position in the world of the chunk (corner)
	const int worldYCorner = static_cast<int>((*offset).y) * ChunkSettings::CHUNK_SIZE;

	// Map for any blocks attempting to be placed outside of this chunk
	BlockQueueMap outerChunksQueue;

	// The chunk has not been created yet so initially use a full array
	// and convert to appropriate storage type from generation result
	chunkBlocks = new ChunkSettings::ChunkBlockValueFull;
	// Reference to actual block array
	ChunkSettings::blockArray& blockArray = dynamic_cast<ChunkSettings::ChunkBlockValueFull*>(chunkBlocks)->chunkBlocks;

	// Counter of how many total blocks are air
	std::uint32_t airCounter = 0u;

	for (int z = 0, perlin = 0; z < ChunkSettings::CHUNK_SIZE; ++z) {
		//const int zIndex = z << ChunkSettings::CHUNK_SIZE_POW2; // The Z position index for block array
		for (int x = 0; x < ChunkSettings::CHUNK_SIZE; ++x) {
			//const int xIndex = x << ChunkSettings::CHUNK_SIZE_POW; // The X position index for block array

			// Chunks with the same XZ offset (different Y) will use the same XZ values for the perlin noise calculation
			// and so will get the same result each time, so it's much better to precalculate the full chunk values
			const WorldPerlin::NoiseResult& positionNoise = perlinResults[perlin++];
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
					} else AttemptGenerateTree(outerChunksQueue, x, y, z, positionNoise, ObjectID::Log, ObjectID::Leaves);
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

				// Finally, set the block at the corresponding array index(es)
				//blockDataArray[y + xIndex + zIndex] = finalBlock;
				blockArray[x][y][z] = finalBlock;
			}
		}
	}

	// If the chunk is just air, there is no need to store every single block
	if (airCounter == ChunkSettings::UCHUNK_BLOCKS_AMOUNT) {
		delete chunkBlocks;
		chunkBlocks = new ChunkSettings::ChunkBlockAir;
	}

	return outerChunksQueue;
}

void Chunk::AttemptGenerateTree(BlockQueueMap& treeBlocksQueue, int x, int y, int z, const WorldPerlin::NoiseResult& noise, ObjectID logID, ObjectID leavesID) noexcept
{
	if (NoiseValueRand(noise.flatness * (noise.humidity / noise.temperature - 1.0f), 1000)) return;

	const int treeHeight = static_cast<int>(noise.flatness * 3.0f) + 5;
	const WorldPos above = *offset + ChunkSettings::worldDirections[WorldDirectionInt::IWorldDir_Up];

	// Add logs - for now they will tear through anything in their path
	for (int logY = y + 1, logTop = logY + treeHeight; logY < logTop; ++logY) {
		if (logY >= ChunkSettings::CHUNK_SIZE) treeBlocksQueue[above].emplace_back(BlockQueue(x, Math::loopAround(logY, 0, 32), z, logID));
		else chunkBlocks->SetBlock(x, logY, z, logID);
	}

	// Use bitwise check to quickly determine if leaf position is inside chunk
	const int allBitsExceptMax = ~ChunkSettings::CHUNK_SIZE_M1;

	// Leaf placing lambda (place in queue if the leaf is outside local chunk)
	const auto PossibleSpawnLeaf = [&](int leavesX, int leavesY, int leavesZ) {
		if ((leavesX | leavesY | leavesZ) & allBitsExceptMax) {
			const WorldPos newChunkOffset = *offset + ChunkSettings::WorldPositionToOffset(leavesX, leavesY, leavesZ);
			treeBlocksQueue[newChunkOffset].emplace_back(BlockQueue(Math::loopAround(leavesX, 0, 32), Math::loopAround(leavesY, 0, 32), Math::loopAround(leavesZ, 0, 32), leavesID));
		} else if (ChunkSettings::GetBlockData(chunkBlocks->GetBlock(leavesX, leavesY, leavesZ)).natureReplaceable) chunkBlocks->SetBlock(leavesX, leavesY, leavesZ, leavesID);
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

				if (midX && midZ) continue; // Don't attempt to place where logs are guaranteed
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

bool Chunk::NoiseValueRand(float noiseVal, int oneInX) noexcept
{
	// I don't know if this works or how it works, but Wikipedia said so.
	// https://en.wikipedia.org/wiki/Xorshift

	// Big number multiplier as this only works with proper integer values (noise values will always cast to 0 otherwise)
	unsigned mult = static_cast<unsigned>(noiseVal * 8278555403.357f);

	mult ^= mult << 13u;
	mult ^= mult >> 17u;
	mult ^= mult << 5;
	return mult % oneInX;
}

void Chunk::CalculateChunk(const ChunkGetter& findFunction) noexcept
{
	if (chunkBlocks == nullptr) { 
		TextFormat::warn("\nAttempted to calculate chunk with null blocks\n", "Null calculation"); 
		return;
	} else if (ChunkSettings::IsAirChunk(chunkBlocks)) return;

	// Get reference to block data, avoiding doing 'GetBlock' calls (function call overhead)
	const ChunkSettings::blockArray& blockArray = ChunkSettings::GetFullBlockArray(chunkBlocks)->chunkBlocks;

	// Store nearby chunks in an array for easier access (last index is current chunk)
	const ChunkSettings::blockArray* localNearby[7]{};
	localNearby[6] = &blockArray;

	for (int localNearbyIndex = 0; localNearbyIndex < 6; ++localNearbyIndex) {
		// Look for a chunk in each direction
		const WorldPos& direction = ChunkSettings::worldDirections[localNearbyIndex];
		const Chunk* nearbyChunk = findFunction(*offset + direction);

		// If the chunk exists and uses a 'full block array', add it to the nearby chunk blocks array
		if (nearbyChunk != nullptr) {
			const ChunkSettings::ChunkBlockValueFull* blocks = ChunkSettings::GetFullBlockArray(nearbyChunk->chunkBlocks);
			if (blocks != nullptr) {
				localNearby[localNearbyIndex] = &blocks->chunkBlocks;
				continue;
			}
		}

		// If not, use the default empty chunk
		localNearby[localNearbyIndex] = &ChunkSettings::emptyChunk;
	}

	// Create an array with the maximum size on the outside to be used per chunk face calculation
	std::uint32_t* outer = new std::uint32_t[ChunkSettings::CHUNK_BLOCKS_AMOUNT];

	// Loop through each array in the given face data array 
	std::size_t lookupIndex = 0u;
	for (std::size_t faceIndex = 0; faceIndex < 6u; ++faceIndex) {
		FaceAxisData& faceData = chunkFaceData[faceIndex];

		// Reset any previous data
		faceData.translucentFaceCount = 0u;
		faceData.faceCount = 0u;

		// Loop through all the chunk's blocks for each face direction
		for (int x = 0; x < ChunkSettings::CHUNK_SIZE; ++x) {
			const ObjectID (&outerBlockArray)[ChunkSettings::CHUNK_SIZE][ChunkSettings::CHUNK_SIZE] = blockArray[x];

			for (int y = 0; y < ChunkSettings::CHUNK_SIZE; ++y) {
				const std::uint32_t yIndex = y << ChunkSettings::CHUNK_SIZE_POW;
				const ObjectID (&innerBlockArray)[ChunkSettings::CHUNK_SIZE] = outerBlockArray[y];

				for (int z = 0; z < ChunkSettings::CHUNK_SIZE; ++z) {
					// Get properties of the current block
					const WorldBlockData& currentBlockData = ChunkSettings::GetBlockData(innerBlockArray[z]);
					// Get precalculated results for this position index and face
					const ChunkLookupTable::ChunkLookupData& nearbyData = chunkLookupData.lookupData[lookupIndex++];
					// Get the data of the block next to the current face, which could be in this or a surrounding chunk
					const WorldBlockData& nextBlock = ChunkSettings::GetBlockData((*localNearby[nearbyData.nearbyIndex])[nearbyData.nextPos.x][nearbyData.nextPos.y][nearbyData.nextPos.z]);

					// Check if the face is not obscured by the block found to be next to it
					if (currentBlockData.notObscuredBy(currentBlockData, nextBlock)) {
						const std::uint32_t zyIndex = yIndex + (z << ChunkSettings::CHUNK_SIZE_POW2);
						// Compress the position and texture data into one integer
						// Layout: TTTT TTTH HHHH WWWW WZZZ ZZYY YYYX XXXX
						const std::uint32_t positionIndex = zyIndex + static_cast<std::uint32_t>(x);
						const std::uint32_t textureData = static_cast<std::uint32_t>(currentBlockData.textures[faceIndex]) << 25u;
						const std::uint32_t sizeData = (1u << 15u) + (1u << 20u);
						const std::uint32_t newData = positionIndex + sizeData + textureData;

						// Blocks with transparency need to be rendered last for them to be rendered
						// correctly on top of existing terrain, so they can be placed starting from
						// the end of the data array instead to be seperated from the opaque blocks
						if (currentBlockData.hasTransparency) {
							// Set the data in reverse order, starting from the end of the array 
							// (pre-increment to avoid writing to out of bounds the first time)
							const std::uint16_t reverseIndex = ChunkSettings::U16CHUNK_BLOCKS_AMOUNT - ++faceData.translucentFaceCount;
							outer[reverseIndex] = newData;
						}
						else {
							// Append the new data to the current chunk face data and increment normal face count
							outer[faceData.faceCount++] = newData;
						}
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
			outer + (ChunkSettings::U16CHUNK_BLOCKS_AMOUNT - faceData.translucentFaceCount),
			sizeof(std::uint32_t) * faceData.translucentFaceCount
		);

		// The data now is stored as such (assuming no bugs):
		// [(opaque data)(translucent data)] <--- No gaps, exact size is allocated
	}

	// Ensure outer data array is cleaned up
	delete[] outer;
}


void Chunk::ChunkBinaryGreedMeshing(const ChunkGetter&) noexcept
{
	// TODO: Simple algorithm that will most likely be optimized to the ground later (**if** it works)
	// Note 28/5/2025: 'Simple'???

	if (ChunkSettings::IsAirChunk(chunkBlocks)) return;
	if (chunkBlocks == nullptr) {
		TextFormat::warn("\nAttempted to calculate chunk with null blocks\n", "Null calculation");
		return;
	}

	// Get reference to block data, avoiding doing 'GetBlock' calls (function call overhead)
	const ChunkSettings::blockArray& blockArray = ChunkSettings::GetFullBlockArray(chunkBlocks)->chunkBlocks;

	const int TESTFACEDIRECTION = IWorldDir_Front; // Z+
	FaceAxisData& faceData = chunkFaceData[TESTFACEDIRECTION];
	faceData.instancesData = new std::uint32_t[ChunkSettings::CHUNK_BLOCKS_AMOUNT];
	faceData.faceCount = 0u;
	faceData.translucentFaceCount = 0u;
	
	typedef std::uint32_t StateType;
	//constexpr int nextZIndex = 1 << ChunkSettings::CHUNK_SIZE_POW2;
	constexpr int chunkSizeIndex = ChunkSettings::CHUNK_SIZE - 1;
	constexpr int stateBitsCount = sizeof(StateType[8]);

	// Binary greedy meshing: Combine surrounding faces with same texture to decrease triangle count, using
	// bits to determine visible faces and to advance through the bitmaps and extend faces.

	// If doing meshing on the a specific face axis, then advancing should be done on
	// the two other axis, as faces can only be extended to make larger 2D shapes (cannot extend to make 3D shapes).

	// Possible addition: Meshing in one direction first then extending to another could result in having more
	// faces than meshing in the second direction initially (e.g. extending horizontally then vertically could result
	// in more triangles/shapes than doing it vertically first), so check for best result.

	// Bitmap of all valid blocks (aka faces that should be visible) - meshing in 6 different directions 
	// means that they require a full bitmap for each with all of the different visible faces.

	// TODO: 6 faces (testing 1 first)
	// A 2D array is used as the 3rd axis is determined by each bit in each StateType
	StateType chunkFaceStates[ChunkSettings::CHUNK_SIZE][ChunkSettings::CHUNK_SIZE]{};

	// TESTING ON Z+ FIRST: TRAVERSE THROUGH Y THEN X FOR MESHING
	// best cache locality - avoid large jumps in array lookup by traversing through Y then X then Z:
	// bits 1-5 = Y position, bits 5-10 = X position, bits 10-15 = Z position

	// If you have any questions on how this works, feel free to ask me on GitHub!
	// Or you could just... read the comments?

	for (int outer = 0; outer < ChunkSettings::CHUNK_SIZE; ++outer) {
		//const bool outerOut = outer == chunkSizeIndex;
		StateType (&innerChunkState)[ChunkSettings::CHUNK_SIZE] = chunkFaceStates[outer]; // Reference to 1D array
		for (int inner = 0; inner < ChunkSettings::CHUNK_SIZE; ++inner) {
			const bool innerOut = inner == chunkSizeIndex;
			StateType& bitmap = innerChunkState[inner]; // Bitmap of final axis
			for (int inc = 0; inc < ChunkSettings::CHUNK_SIZE; ++inc) {
				//const bool incOut = inc == chunkSizeIndex;
				int shift = static_cast<int>(innerOut);
				// Visibility checks
				if (!shift) {
					// Check if face is obscured by another block in this chunk
					//const int blockIndex = zxIndex + y; 
					const WorldBlockData& currentProperties = ChunkSettings::GetBlockData(blockArray[outer][inc][inner]);
					const WorldBlockData& otherProperties = ChunkSettings::GetBlockData(blockArray[outer][inc][inner]);
					shift = currentProperties.notObscuredBy(currentProperties, otherProperties);
				}
				
				// Only set bit if the current face is not obscured by any other face (shift = 1)
				bitmap |= (shift << inc);
			}
		}
	}

	std::uint32_t* currentFaceData = new std::uint32_t[ChunkSettings::CHUNK_BLOCKS_AMOUNT];

	const int p2 = ChunkSettings::CHUNK_SIZE_POW2;
	const int p1 = ChunkSettings::CHUNK_SIZE_POW;
	const int p0 = 0;

	const int outerAxisShift = p0;
	const int innerAxisShift = p1;
	const int incrementShift = p2;

	// Go through each bit in the chunk state, building faces from adjacent bits that refer to the same block type
	for (int outerAxisIndex = 0; outerAxisIndex < ChunkSettings::CHUNK_SIZE; ++outerAxisIndex) {
		StateType (&innerState)[ChunkSettings::CHUNK_SIZE] = chunkFaceStates[outerAxisIndex]; // Reference to 1D array
		//const int zIndex = outerAxisIndex << ChunkSettings::CHUNK_SIZE_POW2;
		for (int innerAxisIndex = 0; innerAxisIndex < ChunkSettings::CHUNK_SIZE; ++innerAxisIndex) {
			//const int xIndex = innerAxisIndex << ChunkSettings::CHUNK_SIZE_POW;
			StateType& state = innerState[innerAxisIndex]; // Unsigned value with set bits determining visible faces
			int incrementAxisIndex = 0; // What bit is going to be checked

			// Do greedy meshing whilst the current bitmap has some bits set -> valid blocks are present
			while (state) {
				if (!(state & (1 << incrementAxisIndex))) { 
					incrementAxisIndex++; 
					continue;  // The current bit is not set -> no valid block here, check the next one
				}

				// Block array index of two axis from 'for' loops above
				//const int blocksZXIndex = zIndex + xIndex;

				// Get the current block from the state and bit indexes
				const ObjectID (&blocksAxisLine)[ChunkSettings::CHUNK_SIZE] = blockArray[outerAxisIndex][innerAxisIndex]; 
				ObjectID blockType = blocksAxisLine[incrementAxisIndex];
				const int incrementAxisStartIndex = incrementAxisIndex; // Save initial axis index for later
				int width = 1, height = 1; // Width and height of the resulting face, will increase if space is found
				
				// The logic here is quite similar to what is done in the below loops, but since
				// it is the first one, it determines what face is going to be extended and the block type.
				// The below loops also clear the bits, but they are checking if they refer to the above type.

				state &= ~(1u << incrementAxisIndex++); // Clear the current bit
				
				// Loop through Z axis seperately from the above while loop to check for current block type
				// instead of just checking for any set bits (extending face 'width' now)
				while (state) {
					if (!(state & (1 << incrementAxisIndex))) {
						incrementAxisIndex++;
						break; // Invalid block found (bit not set): face width cannot be exteneded
					}
					
					// Also stop extending if the next valid block turns out to be of a different block type
					if (blocksAxisLine[incrementAxisIndex++] != blockType) break; 

					state &= ~(1 << incrementAxisIndex); // Clear bit as it has been searched, move on to next bit
					width++; // Increase current face width
				}

				// Check for valid blocks in other axis (extending face 'height' now)
				// This 'for' loop does the same 'clearing' logic as above, but since
				// the 'width'/amount of bits to check are known, it is done all at once.
				for (int nextStateX = innerAxisIndex + 1; nextStateX < ChunkSettings::CHUNK_SIZE; ++nextStateX) {
					// A similar searching as seen above, however the other axis is 
					// checked now, which is located on the next face 'state' value.
					StateType& nextState = innerState[nextStateX];

					/* 
						Creates a bitmask that checks 'width' amount of bits starting from specific index by:
						Using a full bitmask initially: 0b11111111....
						Shifting the full bitmask so only 'width' amount of bits are set: 0b00...000111 (width = 3)
						Shifting the bitmask again the other way by the bit index: 0b00...011100 (index = 2)
					*/

					constexpr StateType allOnesBitmask = ~0u; // Full bitmask
					const StateType bitsCheck = (allOnesBitmask >> (stateBitsCount - width)) << incrementAxisStartIndex;

					// Check if all bits are set if the bitwise AND operation returns the same as the bitmask
					// This skips having to check if each block is the same when an invalid block is present.
					if ((nextState & bitsCheck) != bitsCheck) break;

					// Check if each corresponding block on the next face state array has the same type
					// (checking the same amount of blocks from the same starting index to see if the face
					// can be extended on this axis)
					bool allValid = true;
					for (int nextIncrementAxisIndex = incrementAxisStartIndex, end = incrementAxisIndex; nextIncrementAxisIndex < end; ++nextIncrementAxisIndex) {
						if (blocksAxisLine[nextIncrementAxisIndex] == blockType) continue;
						// Different block found - exit out of both loops to stop trying to extend height
						allValid = false;
						break;
					}

					if (!allValid) break; // Not all blocks match the block type, face 'height' cannot be extended
					height++; // All of the blocks are the same, increase the height

					// Clear all of the bits specified in the bits checked above for this if statement
					// This is done by doing the bitwise AND for all of the other bits that should be kept
					// Example: 0b011011101 & ~0b000011100 = 0b011011101 & 0b111100011 = 0b011000001
					nextState &= ~bitsCheck;
				}

				const WorldBlockData blockData = ChunkSettings::GetBlockData(blockType); // Get block properties
				const TextureIDTypeof textureIndex = blockData.textures[TESTFACEDIRECTION]; // Get specific texture of block
				
				// Face data layout: TTTT TTTH HHHH WWWW WZZZ ZZXX XXXY YYYY
				// XYZ: position, WH: size (width and height), T: texture index
				// TODO: possibly have more bits for texture in the future 
				const std::uint32_t newFaceData = 
					static_cast<std::uint32_t>(
						(incrementAxisStartIndex << incrementShift) + 
						(innerAxisIndex << innerAxisShift) + 
						(outerAxisIndex << outerAxisShift) + 
						(width << 15) + 
						(height << 20)
					) + (static_cast<std::uint32_t>(textureIndex) << 25u);

				// Blocks with transparency need to be rendered last for them to be rendered
				// correctly on top of existing terrain, so they can be placed starting from
				// the end of the data array instead to be seperated from the opaque blocks
				if (blockData.hasTransparency) {
					// Set the data in reverse order, starting from the end of the array 
					// (pre-increment to avoid writing to out of bounds the first time)
					const int reverseIndex = ChunkSettings::CHUNK_BLOCKS_AMOUNT - ++faceData.translucentFaceCount;
					currentFaceData[reverseIndex] = newFaceData;
				}
				else {
					// Append the new data to the current chunk face data and increment face index
					currentFaceData[faceData.faceCount++] = newFaceData;
				}
			}
		}
	}
	
	// Compress the array as there is most likely a gap between the normal face data
	// and the translucent faces as the total face count is known now
	const std::size_t totalFaces = faceData.TotalFaces<std::size_t>();

	if (!totalFaces) return; // No need for compression if there is no data in the first place

	// Allocate the exact amount of data needed to store all the faces
	faceData.instancesData = new std::uint32_t[totalFaces];

	// Copy the opaque and translucent sections of the face data to be 
	// next to each other in the new compressed array

	// Opaque face data
	std::memcpy(faceData.instancesData, currentFaceData, sizeof(std::uint32_t) * faceData.faceCount);

	// Translucent face data
	std::memcpy(
		faceData.instancesData + faceData.faceCount,
		currentFaceData + (ChunkSettings::CHUNK_BLOCKS_AMOUNT - faceData.translucentFaceCount),
		sizeof(std::uint32_t) * faceData.translucentFaceCount
	);

	// The data now is stored as such (assuming no bugs):
	// [(opaque data)(translucent data)] <--- No gaps, exact size is allocated
}

Chunk::~Chunk()
{
	// Remove instance face data (if any)
	for (FaceAxisData& fd : chunkFaceData) if (fd.instancesData != nullptr) delete[] fd.instancesData;
	// Delete heap allocated chunk block data
	delete chunkBlocks;
}
