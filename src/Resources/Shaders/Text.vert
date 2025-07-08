#version 430 core

layout (location = 0) in vec2 baseGUI;
layout (location = 1) in uint baseIndex;

layout (location = 2) in vec4 d1; // X, Y, width, height
layout (location = 3) in uvec2 d2; // Character, colour

uniform float texturePositions[191];

out vec2 t;
out vec4 c;

void main()
{
	gl_Position = vec4(
		d1.x + (baseGUI.x * d1.z),
		d1.y - (baseGUI.y * d1.w),
		0.0, 1.0
	);
	
	t = vec2(texturePositions[d2.x + baseIndex], baseGUI.y);
	c = vec4(
		(d2.y & 0xFF) / 255.0, 
		((d2.y >> 8) & 0xFF) / 255.0, 
		((d2.y >> 16) & 0xFF) / 255.0,
		((d2.y >> 24) & 0xFF) / 255.0
	);
}
