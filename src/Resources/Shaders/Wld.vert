#version 430 core
#extension GL_ARB_shader_draw_parameters : require
#extension GL_NV_gpu_shader5 : require

struct ChunkOffset {
	i64vec3 v;
	int8_t f;
};

layout (std430, binding = 0) readonly buffer chunkMeshData {
	ChunkOffset chunkData[];
};

layout (location = 0) in uint data;
// TTTT TTTH HHHH WWWW WZZZ ZZXX XXXY YYYY
layout (location = 1) in ivec4 baseXZ;
layout (location = 2) in ivec4 baseYZ;
layout (location = 3) in ivec4 baseYW;

layout (std140, binding = 0) uniform GameMatrices {
	mat4 matrix;
	mat4 originMatrix;
	mat4 starMatrix;
};

layout (std140, binding = 1) uniform GameTimes {
	float time;
	float fogTime;
	float starTime;
	float gameTime;
	float cloudsTime;
};

layout (std140, binding = 2) uniform GameColours {
	vec4 mainSkyColour;
	vec4 eveningSkyColour;
	vec4 worldLight;
};

layout (std140, binding = 3) uniform GamePositions {
	dvec4 raycastBlockPosition;
	dvec4 playerPosition;
};

layout (std140, binding = 4) uniform GameSizes {
	float blockTextureSize;
	float inventoryTextureSize;
	float screenAspect;
	float textWidth;
	float textHeight;
	float inventoryWidth;
	float inventoryHeight;
};

out vec2 TexCoord;
out vec4 mult;

void main()
{
	TexCoord = vec2((baseYZ.x + (data >> 25)) * blockTextureSize, baseYW.y);
	mult = worldLight;
	const ChunkOffset cd = chunkData[gl_DrawIDARB];
	
	dvec3 blockPos = dvec3(
		(data >> 5) & 31,
		data & 31,
		(data >> 10) & 31
	) + cd.v;

		 if (cd.f == 0) blockPos += baseYZ.ywx;	// Y+
	else if (cd.f == 1) blockPos += baseYW.yzx;	// Y-
	else if (cd.f == 2) blockPos += baseXZ.wyx;	// X+
	else if (cd.f == 3) blockPos += baseYZ.zyx;	// X-
	else if (cd.f == 4) blockPos += baseYZ.xyw;	// Z+
	else blockPos += baseXZ.xyz;				// Z-
	
	gl_Position = originMatrix * vec4(blockPos - playerPosition.xyz, 1.0);
}
