#version 430 core

layout (location = 0) in vec3 baseData;
layout (location = 1) in vec3 data; // X, Z, W

layout (std140, binding = 0) uniform GameMatrices {
	mat4 originMatrix;
	mat4 starMatrix;
	mat4 planetsMatrix;
};

layout (std140, binding = 1) uniform GameTimes {
	float time;
	float fogEnd;
	float fogRange;
	float starTime;
	float gameTime;
	float cloudsTime;
};

layout (std140, binding = 3) uniform GamePositions {
	dvec4 raycastBlockPosition;
	dvec4 playerPosition;
};

out vec4 col;

void main()
{
	const vec3 baseDataMult = baseData * data.z;
	const vec3 relPos = vec3(dvec3(
		data.x + baseDataMult.x,
		198.7 + (gl_InstanceID * 0.05) + baseDataMult.y,
		data.y + baseDataMult.z + gameTime
	) - playerPosition.xyz);

	gl_Position = originMatrix * vec4(relPos, 1.0);
	col = vec4(vec3(cloudsTime), clamp((fogEnd - length(relPos.xz)) / fogRange, 0.0, 1.0));
}
