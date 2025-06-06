#version 420 core

layout (location = 0) in vec3 baseData;
layout (location = 1) in vec3 data; // X, Z, W

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

out vec4 c;
const vec2 cflts = vec2(0.8, 0.0);

void main()
{
	const vec3 baseDataMult = baseData * data.z;
	const dvec3 basePos = vec3(
		data.x + baseDataMult.x,
		198.7 + (gl_InstanceID * 0.05) + baseDataMult.y,
		data.y + baseDataMult.z + gameTime
	);

	gl_Position = originMatrix * vec4(basePos - playerPosition.xyz, 1.0);
	c = vec4(cloudsTime) * cflts.xxxy + cflts.yyyx;
}
