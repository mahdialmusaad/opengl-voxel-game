#version 430 core

layout (std140, binding = 2) uniform GameColours {
	vec4 mainSkyColour;
	vec4 eveningSkyColour;
	vec4 worldLight;
};

in vec4 col;
out vec4 f;

void main() {
	f = vec4(mix(mainSkyColour, vec4(col.xyz, 1.0), col.w));
	if (col.w == 0.0) discard; 
}