#version 420 core
layout (location = 0) in vec3 basePos;

layout (std140, binding = 0) uniform GameMatrices {
	mat4 matrix;
	mat4 originMatrix;
	mat4 starMatrix;
};

layout (std140, binding = 3) uniform GamePositions {
	dvec4 relativeRaycastBlockPosition;
	dvec4 playerPosition;
};

void main()
{
	gl_Position = originMatrix * vec4(basePos + relativeRaycastBlockPosition.xyz, 1.0);
}
