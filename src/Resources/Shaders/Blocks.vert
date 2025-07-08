#version 430 core
#extension GL_ARB_shader_draw_parameters : require

struct ChunkOffset {
	double x;
	double z;
	float y;
	int f;
};

layout (std430, binding = 0) readonly buffer chunkMeshData {
	ChunkOffset chunkData[];
};

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
};

layout (location = 0) in uint data;
// TTTT TTTT TTTT TTTT TZZZ ZZYY YYYX XXXX
layout (location = 1) in vec4 baseXZ;
layout (location = 2) in vec4 baseYZ;
layout (location = 3) in vec4 baseYW;

out vec2 TexCoord;
out vec4 mult;

void main()
{
	const ChunkOffset cd = chunkData[gl_DrawIDARB];

	dvec3 blockPos = dvec3(
		data & 31,
		(data >> 5) & 31,
		(data >> 10) & 31
	) + dvec3(cd.x, cd.y, cd.z);

		 if (cd.f == 0) blockPos += baseXZ.wyx;	// X+
	else if (cd.f == 1) blockPos += baseYZ.zyx;	// X-
	else if (cd.f == 2) blockPos += baseYZ.ywx;	// Y+
	else if (cd.f == 3) blockPos += baseYW.yzx;	// Y-
	else if (cd.f == 4) blockPos += baseYZ.xyw;	// Z+
	               else blockPos += baseXZ.xyz;	// Z-
	
	gl_Position = originMatrix * vec4(blockPos - playerPosition.xyz, 1.0);
	TexCoord = vec2((baseYZ.x + float(data >> 15)) * blockTextureSize, baseYW.y);
	mult = worldLight;
}
