#version 430 core

layout (binding = 0) uniform sampler2D blocksTexture;

in vec3 texAndFactor;
out vec4 resColour;

layout (std140, binding = 2) uniform GameColours {
	vec4 mainSkyColour;
	vec4 eveningSkyColour;
	vec4 worldLight;
};

void main()
{
	resColour = mix(mainSkyColour, texture(blocksTexture, texAndFactor.xy) * worldLight, texAndFactor.z);
	if (resColour.a == 0.0) discard;
}