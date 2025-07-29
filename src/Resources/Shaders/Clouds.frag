#version 430 core

layout (std140, binding = 2) uniform GameColours {
	vec4 mainSkyColour;
	vec4 eveningSkyColour;
	vec4 worldLight;
};

in vec4 col;
out vec4 f;

void main() {
	f = vec4(mix(mainSkyColour.xyz, col.xyz, col.w), 0.8);
	if (f.a == 0.0) discard; 
}