#version 420 core

layout (std140, binding = 0) uniform GameMatrices {
	mat4 matrix;
	mat4 originMatrix;
	mat4 starMatrix;
};

layout (std140, binding = 2) uniform GameColours {
	vec4 mainSkyColour;
	vec4 eveningSkyColour;
	vec4 worldLight;
};

layout (location = 0) in vec3 position;
out vec4 i;

void main()
{
	gl_Position = originMatrix * vec4(position, 0.0);
	i = mix(mainSkyColour, mainSkyColour, abs(position.y) * 10.0);
}
