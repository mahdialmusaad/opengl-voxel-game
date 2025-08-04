#version 430 core

layout (std140, binding = 0) uniform GameMatrices {
	mat4 originMatrix;
	mat4 starMatrix;
	mat4 planetsMatrix;
	mat4 skyMatrix;
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

layout (location = 0) in vec3 position;
out vec4 i;

void main()
{
	gl_Position = (skyMatrix * vec4(position, 0.0)).xyzz;
	i = mix(mainSkyColour, eveningSkyColour, 1.0f - abs(position.y * 10.0));
}
