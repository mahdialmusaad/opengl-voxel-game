#version 420 core

layout (binding = 0) uniform sampler2D blockTexturesID;
layout (binding = 2) uniform sampler2D textureID;

out vec4 FragColor;
in vec2 TexCoord;
in flat int which;

void main() {
	FragColor = texture(which == 0 ? textureID : blockTexturesID, TexCoord);
}
