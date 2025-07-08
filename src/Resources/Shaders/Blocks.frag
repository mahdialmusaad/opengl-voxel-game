#version 430 core

layout (binding = 0) uniform sampler2D blocksTexture;

out vec4 FragColor;
in vec2 TexCoord;
in vec4 mult;

void main()
{
	FragColor = texture(blocksTexture, TexCoord) * mult;
	if (FragColor.a == 0.0) discard;
}