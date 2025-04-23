#version 420 core

layout (location = 0) in uint data;
// WWWW WWWW ZZZZ ZZZZ ZZZZ XXXX XXXX XXXX
layout (location = 1) in vec3 baseData;

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

layout (std140, binding = 3) uniform GamePositions {
	dvec4 raycastBlockPosition;
	dvec4 playerPosition;
};

out vec4 col;
const vec2 cflts = vec2(0.8, 0.0);

void main()
{
	const float x = (data & 0xfff) - 2047.5;
	const float z = (data >> 12 & 0xfff) - 2047.5;
	const float w = data >> 26;

	gl_Position = originMatrix * vec4(
		dvec3(
			x + (baseData.x * w), 
			(gl_InstanceID * 0.001 + baseData.y) * w + 199.7, 
			z + gameTime + (baseData.z * w)
		) - playerPosition.xyz, 
		1.0
	);

	col = vec4(cloudsTime) * cflts.xxxy + cflts.yyyx;
}
