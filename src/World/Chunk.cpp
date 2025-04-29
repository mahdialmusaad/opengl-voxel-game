#include "Chunk.hpp"

Chunk::Chunk(
	WorldPos offset,
	WorldPerlin::NoiseResult* perlinResults
) noexcept :
	offsetX(offset.x),
	offsetY(narrow_cast<std::int8_t>(offset.y)),
	offsetZ(offset.z),
	positionMagnitude(sqrt(static_cast<double>((offset.x * offset.x) + (offset.y * offset.y) + (offset.z * offset.z))))
{
	//const WorldPos worldPosition = ChunkSettings::GetChunkCorner(offset);
	//const glm::vec3 center = static_cast<glm::vec3>(ChunkSettings::GetChunkCenter(offset));
	const int worldYCorner = static_cast<int>(offsetY) * ChunkSettings::CHUNK_SIZE;

	// The chunk has not been created yet so initially use a full array
	// and convert to appropriate storage type from generation result
	ChunkSettings::ChunkBlockValueFull* fullBlockArray = new ChunkSettings::ChunkBlockValueFull;
	// Reference to actual block array to avoid having to call .SetBlock*() --> function overhead
	ChunkSettings::blockArray& blockDataArray = fullBlockArray->chunkBlocks;

	// Counter of how many total blocks are air
	std::uint32_t airCounter = 0u;

	for (int z = 0, perlin = 0; z < ChunkSettings::CHUNK_SIZE; ++z) {
		const int zIndex = z << ChunkSettings::CHUNK_SIZE_POW2; // The Z position index for block array
		for (int x = 0; x < ChunkSettings::CHUNK_SIZE; ++x) {
			const int xIndex = x << ChunkSettings::CHUNK_SIZE_POW; // The X position index for block array

			// Chunks with the same XZ offset (different Y) will use the same XZ values for the perlin noise calculation
			// and so will get the same result each time, so it's much better to precalculate the full chunk values
			const WorldPerlin::NoiseResult& heightNoise = perlinResults[perlin++];

			// Calculated height at this XZ position using the precalculated noise value
			const int terrainHeight = static_cast<int>(heightNoise.continentalnessHeight);

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
					}
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

				// Finally, set the block at the corresponding array index 
				blockDataArray[y + xIndex + zIndex] = finalBlock;
			}
		}
	}

	// If the chunk is just air, there is no need to store every single block
	if (airCounter == ChunkSettings::UCHUNK_BLOCKS_AMOUNT) {
		delete fullBlockArray;
		chunkBlocks = new ChunkSettings::ChunkBlockAir;
	}
	else chunkBlocks = fullBlockArray;
}

WorldPos Chunk::GetOffset() const noexcept
{
	// Avoid having to cast Y offset all the time due to it being an 8 bit integer compared to X and Z offsets
	return { offsetX, static_cast<PosType>(offsetY), offsetZ };
}


void Chunk::CalculateChunk(const ChunkGetter& findFunction) noexcept
{
	if (ChunkSettings::IsAirChunk(chunkBlocks)) return;
	if (chunkBlocks == nullptr) { 
		TextFormat::warn("\nAttempted to calculate chunk with null blocks\n", "Null calculation"); 
		return;
	}

	// Get reference to block data, avoiding doing 'GetBlock' calls (function call overhead)
	const ChunkSettings::blockArray& blockArray = ChunkSettings::GetFullBlockArray(chunkBlocks)->chunkBlocks;
	// Empty chunk array in case of no chunk existing in a certain direction
	constexpr const ChunkSettings::blockArray* emptyChunk = &ChunkSettings::emptyChunk;

	// Store nearby chunks in an array for easier access (last index is current chunk)
	const ChunkSettings::blockArray* localNearby[7]{};
	localNearby[6] = &blockArray;

	const WorldPos chunkOffset = GetOffset();
	int localNearbyIndex = 0;

	for (const WorldPos& direction : ChunkSettings::worldDirections) {
		// Look for a chunk in each direction
		const Chunk* nearbyChunk = findFunction(chunkOffset + direction);
		// If the chunk exists and uses a 'full block array', add it to the nearby chunk blocks array
		if (nearbyChunk != nullptr) {
			const ChunkSettings::ChunkBlockValueFull* blocks = ChunkSettings::GetFullBlockArray(nearbyChunk->chunkBlocks);
			if (blocks != nullptr) {
				localNearby[localNearbyIndex++] = &blocks->chunkBlocks;
				continue;
			}
		}
		// If not, use the default empty chunk
		localNearby[localNearbyIndex++] = emptyChunk;
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
		for (int i = 0; i <= ChunkSettings::CHUNK_BLOCKS_AMOUNT_INDEX; ++i) {
			// Get properties of the current block
			const WorldBlockData& currentBlockData = ChunkSettings::GetBlockData(static_cast<int>(blockArray[i]));
			// Get precalculated results for this position index and face
			const ChunkLookupTable::ChunkLookupData& surroundingData = chunkLookupData.lookupData[lookupIndex++];
			// Get the data of the block next to the current face, which could be in this or a surrounding chunk
			const WorldBlockData& targetBlockData = ChunkSettings::GetBlockData((*localNearby[surroundingData.nearbyIndex])[surroundingData.blockIndex]);

			// Check if the face is not obscured by the block found to be next to it
			if (currentBlockData.notObscuredBy(currentBlockData, targetBlockData)) {
				// Compress the position and texture data into one integer
				const std::uint32_t newData = 
					 static_cast<std::uint32_t>(i) + 
					(static_cast<std::uint32_t>(currentBlockData.textures[faceIndex]) << 25u);

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
					// Append the new data to the current chunk face data and increment face index
					outer[faceData.faceCount++] = newData;
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


void Chunk::CalculateChunkGreedy(const ChunkGetter&) noexcept
{
	// TODO: simple algorithm that will most likely be optimized to the ground later (**if** it works)

	// WARNING to 'never-nesters': You might want to close your eyes whilst scrolling down here...
	// Max indentation level: 7 (depends on your editor's indentation size really)
	// Record indentation level from doing logic inside if statements rather than breaking when false: 9
	// Performace is unaffected, but it becomes extremely unreadable when half of the screen is just indents.

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
	constexpr int nextZIndex = 1 << ChunkSettings::CHUNK_SIZE_POW2;
	constexpr int chunkSizeIndex = ChunkSettings::CHUNK_SIZE - 1;
	constexpr int stateBitsCount = sizeof(StateType[CHAR_BIT]);

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

	// If you have any questions on how this works, just create a new discussion on GitHub.
	// You can do so at https://github.com/mahdialmusaad/badcraft/discussions
	
	for (int z = 0; z < ChunkSettings::CHUNK_SIZE; ++z) {
		StateType (&innerChunkState)[ChunkSettings::CHUNK_SIZE] = chunkFaceStates[z]; // Reference to 1D array
		const int zIndex = z << ChunkSettings::CHUNK_SIZE_POW2;
		for (int x = 0; x < ChunkSettings::CHUNK_SIZE; ++x) {
			int zxIndex = zIndex + (x << ChunkSettings::CHUNK_SIZE_POW);
			StateType& bitmap = innerChunkState[x]; // Bitmap of final axis
			for (int y = 0; y < ChunkSettings::CHUNK_SIZE; ++y) {
				// Visibility checks
				int shift = z == chunkSizeIndex; // TODO: include other chunks - assume visible if outside
				if (shift == 0) {
					// Check if face is obscured by another block in this chunk
					const int blockIndex = zxIndex + y; 
					const WorldBlockData& currentProperties = ChunkSettings::GetBlockData(blockArray[blockIndex]);
					const WorldBlockData& otherProperties = ChunkSettings::GetBlockData(blockArray[blockIndex + nextZIndex]);
					shift = currentProperties.notObscuredBy(currentProperties, otherProperties);
				}
				
				// Only set bit if the current face is not obscured by any other face (shift = 1)
				bitmap |= (shift << z);
			}
		}
	}

	std::uint32_t* currentFaceData = new std::uint32_t[ChunkSettings::CHUNK_BLOCKS_AMOUNT];

	// Go through each bit in the chunk state, building faces from adjacent bits that refer to the same block type
	for (int stateZ = 0; stateZ < ChunkSettings::CHUNK_SIZE; ++stateZ) {
		StateType (&innerState)[ChunkSettings::CHUNK_SIZE] = chunkFaceStates[stateZ]; // Reference to 1D array
		const int zIndex = stateZ << ChunkSettings::CHUNK_SIZE_POW2;
		for (int stateX = 0; stateX < ChunkSettings::CHUNK_SIZE; ++stateX) {
			const int xIndex = stateX << ChunkSettings::CHUNK_SIZE_POW;
			StateType& state = innerState[stateX]; // Unsigned value with set bits determining visible faces
			int currentBitIndex = 0; // What bit is going to be checked

			// Do greedy meshing whilst the current bitmap has some bits set - valid blocks present
			while (state) {
				if (!(state & (1 << currentBitIndex))) { 
					currentBitIndex++; 
					continue;  // The current bit is not set - no valid block here, check the next one
				}

				// Block array index of two axis from 'for' loops above
				const int blocksZXIndex = zIndex + xIndex;

				// Get the current block from the state and bit indexes
				ObjectID blockType = blockArray[blocksZXIndex + currentBitIndex];
				const int startingYIndex = currentBitIndex; // Save Z axis for later
				int width = 1, height = 1; // Width and height of the resulting face, will increase if space is found
				
				// The logic here is quite similar to what is done in the below loops, but since
				// it is the first one, it determines what face is going to be extended and the block type.
				// The below loops also clear the bits, but they are checking if they refer to the above type.

				state &= ~(1u << currentBitIndex++); // Clear the current bit as it has been searched
				
				// Loop through Z axis seperately from the above while loop to check for current block type
				// instead of just checking for any set bits (extending face 'width' now)
				while (state) {
					if (!(state & (1 << currentBitIndex))) {
						currentBitIndex++;
						break; // Invalid block found (bit not set): face width cannot be exteneded
					}
					
					// Also stop extending if the next valid block turns out to be of a different block type
					if (blockArray[blocksZXIndex + currentBitIndex++] != blockType) break; 

					state &= ~(1 << currentBitIndex); // Clear bit as it has been searched, move on to next bit
					width++; // Increase current face width
				}

				// Check for valid blocks in other axis (extending face 'height' now)
				// This 'for' loop does the same 'clearing' logic as above, but since
				// the 'width'/amount of bits to check are known, it is done all at once.
				for (int nextStateX = stateX + 1; nextStateX < ChunkSettings::CHUNK_SIZE; ++nextStateX) {
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
					const StateType bitsCheck = (allOnesBitmask >> (stateBitsCount - width)) << startingYIndex;

					// Check if all bits are set if the bitwise AND operation returns the same as the bitmask
					// This skips having to check if each block is the same when an invalid block is present.
					if ((nextState & bitsCheck) != bitsCheck) break;

					// Check if each corresponding block on the next face state array has the same type
					// (checking the same amount of blocks from the same starting index to see if the face
					// can be extended on this axis)
					bool allValid = true;
					for (int nextStateIndex = startingYIndex, end = currentBitIndex; nextStateIndex < end; ++nextStateIndex) {
						if (blockArray[blocksZXIndex + nextStateIndex] == blockType) continue;
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
					static_cast<std::uint32_t>(blocksZXIndex + startingYIndex + (width << 15) + (height << 20)) +
					(static_cast<std::uint32_t>(textureIndex) << 25u);

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
