#version 430 core
layout (location = 0) in vec3 pos;

layout (std140, binding = 0) uniform GameMatrices {
	mat4 originMatrix;
	mat4 starMatrix;
	mat4 planetsMatrix;
};

layout (std140, binding = 3) uniform GamePositions {
	dvec4 raycastBlockPosition;
	dvec4 playerPosition;
};

void main() {
	gl_Position = originMatrix * vec4(vec4(pos, 1.0) - playerPosition);
}