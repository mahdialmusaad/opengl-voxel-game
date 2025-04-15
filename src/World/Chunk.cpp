#include <World/Chunk.hpp>

Chunk::Chunk(
	WorldPos offset,
	WorldPerlin::NoiseResult* perlinResults
) noexcept :
	offsetX(offset.x),
	offsetY(narrow_cast<int8_t>(offset.y)),
	offsetZ(offset.z),
	positionMagnitude(sqrt(static_cast<double>((offset.x * offset.x) + (offset.y * offset.y) + (offset.z * offset.z))))
{
	const WorldPos worldPosition = ChunkSettings::GetChunkCorner(offset);
	const glm::vec3 center = static_cast<glm::vec3>(ChunkSettings::GetChunkCenter(offset));
	const int worldYCorner = static_cast<int>(offsetY) * ChunkSettings::CHUNK_SIZE;

	// The chunk has not been created yet so initially use a full array
	// and convert to appropriate storage type from generation result
	ChunkSettings::ChunkBlockValueFull* fullBlockArray = new ChunkSettings::ChunkBlockValueFull;
	// Reference to actual block array to avoid having to call .SetBlock*() --> function overhead
	ChunkSettings::blockArray& blockDataArray = fullBlockArray->chunkBlocks;

	// Counter of how many total blocks are air
	uint32_t airCounter = 0;

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
	return { offsetX, static_cast<PosType>(offsetY), offsetZ };
}

void Chunk::CalculateChunk(const ChunkGetter& findFunction) noexcept
{
	// If it's an empty chunk, just return as there aren't going to be any faces anyways
	if (ChunkSettings::IsAirChunk(chunkBlocks)) return;
	if (chunkBlocks == nullptr) { 
		TextFormat::warn("\nAttempted to calculate chunk with null blocks\n", "Null calculation"); 
		return;
	}

	// Get reference to block data, avoiding doing 'GetBlock' calls (function call overhead)
	const ChunkSettings::blockArray& inlineBlockArray = ChunkSettings::GetFullBlockArray(chunkBlocks)->chunkBlocks;
	// Empty chunk array in case of no chunk existing in a certain direction
	constexpr const ChunkSettings::blockArray* emptyChunk = &ChunkSettings::emptyChunk;

	// Store nearby chunks in an array for easier access (last index is current chunk)
	const ChunkSettings::blockArray* localNearby[7]{};
	localNearby[6] = &inlineBlockArray;

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

	const DataArrays::ChunkLookupTable::ChunkLookupData* lookupTable = DataArrays::lookupTable.lookupData;

	// Create an array with the maximum size on the outside to be used per chunk face calculation
	uint32_t* outer = new uint32_t[ChunkSettings::CHUNK_BLOCKS_AMOUNT];

	// Loop through each array in the given face data array 
	int32_t lookupIndex = 0;
	for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
		FaceAxisData& faceData = chunkFaceData[faceIndex];

		// Reset any previous data
		faceData.translucentFaceCount = 0ui16;
		faceData.faceCount = 0ui16;

		// Loop through all the chunk's blocks for each face direction
		for (int i = 0; i <= ChunkSettings::CHUNK_BLOCKS_AMOUNT_INDEX; ++i) {
			const int blockID = static_cast<int>(inlineBlockArray[i]);
			// Data for the current selected block
			const WorldBlockData& blockSpecificData = ChunkSettings::GetBlockData(blockID);

			// Get precalculated results for this position and face
			const DataArrays::ChunkLookupTable::ChunkLookupData& surroundingData = lookupTable[lookupIndex++];

			// Get the block next to the current face, which could be in this chunk or one of the surrounding chunks
			const int targetBlock = static_cast<int>((*localNearby[surroundingData.nearbyIndex])[surroundingData.blockIndex]);

			// Check if the face is not obscured by the block found to be next to it
			if (blockSpecificData.renderFunction(blockID, targetBlock)) {
				/*
					Translucent blocks need to be rendered last, so they need to be separated from the normal faces.
					A way to do this without having to store double the amount of memory is to place them at the end of the data array
					instead (back-front) and store how many there are (no overlaps occur as the max face array is allocated)
				*/

				// Compress the position and texture data into one integer
				const uint32_t newData = 
					 static_cast<uint32_t>(i) + 
					(static_cast<uint32_t>(blockSpecificData.textures[faceIndex]) << 25ui32);

				// Append the integer to the current block data array for this chunk face direction 
				// in the correct location depending on whether it is translucent or not.
				if (blockSpecificData.hasTransparency) {
					// Set the data in reverse order, starting from the end of the array 
					// Pre-increment instead as indexing starts from 0 and ends at size - 1) 
					// (avoid writing to array[size], which is out of bounds)
					uint16_t reverseIndex = ChunkSettings::U16CHUNK_BLOCKS_AMOUNT - ++faceData.translucentFaceCount;
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
		const size_t totalFaces = faceData.TotalFaces<size_t>();
		if (!totalFaces) continue; // No need for compression if there is no data in the first place

		// Allocate the exact amount of data needed to store all the faces
		faceData.instancesData = new uint32_t[totalFaces];

		// Copy the opaque and translucent sections of the face data to be 
		// next to each other in the new compressed array

		// Opaque face data
		memcpy(
			faceData.instancesData,
			outer,
			static_cast<size_t>(faceData.faceCount) * sizeof(uint32_t)
		);
		// Translucent face data
		memcpy(
			faceData.instancesData + faceData.faceCount,
			outer + (ChunkSettings::U16CHUNK_BLOCKS_AMOUNT - faceData.translucentFaceCount),
			static_cast<size_t>(faceData.translucentFaceCount) * sizeof(uint32_t)
		);

		/*
			Data before:
			[(opaque faces)(possible gap)(translucent faces)]

			Data now (compressed = cool memory savings + easier to use):
			[(opaque faces)(translucent faces)]
		*/
	}

	// Ensure outer data array is cleaned up
	delete[] outer;
}

void Chunk::CalculateChunkGreedy(const ChunkGetter&) noexcept
{
	// If it's an empty chunk, just return as there aren't going to be any faces anyways
	if (ChunkSettings::IsAirChunk(chunkBlocks)) return;
	if (chunkBlocks == nullptr) {
		TextFormat::warn("\nAttempted to calculate chunk with null blocks\n", "Null calculation");
		return;
	}

	// Get reference to block data, avoiding doing 'GetBlock' calls (function call overhead)
	const ChunkSettings::blockArray& inlineBlockArray = ChunkSettings::GetFullBlockArray(chunkBlocks)->chunkBlocks;

	FaceAxisData& faceData = chunkFaceData[IWorldDir_Front];
	faceData.instancesData = new uint32_t[ChunkSettings::CHUNK_BLOCKS_AMOUNT];
	faceData.faceCount = 0ui16;
	faceData.translucentFaceCount = 0ui16;
	
	typedef uint32_t StateType;
	constexpr int stateBits = sizeof(StateType) * 8;

	StateType initialChunkState[ChunkSettings::CHUNK_SIZE][ChunkSettings::CHUNK_SIZE]{};
	for (int y = 0; y < ChunkSettings::CHUNK_SIZE; ++y) {
		for (int x = 0; x < ChunkSettings::CHUNK_SIZE; ++x) {
			int xyIndex = x + (y << ChunkSettings::CHUNK_SIZE_POW);
			StateType& bitmap = initialChunkState[y][x];
			for (int z = 0; z < ChunkSettings::CHUNK_SIZE; ++z) {
				int orShift = inlineBlockArray[xyIndex + (z << ChunkSettings::CHUNK_SIZE_POW2)] != ObjectID::Air ? 1 : 0;
				bitmap |= (orShift << z);
			}
		}
	}

	// chunk state is 2d StateType array in XY axis with each bit showing whether there is a valid block in the Z axis

	StateType editState[ChunkSettings::CHUNK_SIZE][ChunkSettings::CHUNK_SIZE];
	memcpy(editState, initialChunkState, sizeof(initialChunkState));

	for (int stateY = 0; stateY < stateBits; ++stateY) {
		for (int stateX = 0; stateX < stateBits; ++stateX) {
			StateType& state = editState[stateY][stateX];
			if (!state) continue; // all air
		}
	}
}

Chunk::~Chunk()
{
	// Remove instance face data (if any)
	for (FaceAxisData& fd : chunkFaceData) if (fd.instancesData != nullptr) delete[] fd.instancesData;
	// Delete heap allocated chunk block data
	delete chunkBlocks;
}
