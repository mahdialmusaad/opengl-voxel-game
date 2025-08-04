#version 430 core

layout (location = 0) in vec3 baseData;
layout (location = 1) in vec4 data; // X, Y, Z, W 

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
	const vec3 baseDataMult = baseData * data.w;
	const vec4 relPos = vec4(dvec3(
		data.x + baseDataMult.x,
		data.y + baseDataMult.y,
		data.z + baseDataMult.z + gameTime
	) - playerPosition.xyz, 1.0);

	gl_Position = originMatrix * relPos;
	col = vec4(vec3(cloudsTime), clamp((fogEnd - length(relPos.xyz - vec3(0.0, relPos.y * 0.75, 0.0)) * 0.33) / fogRange, 0.0, 0.8));
}
