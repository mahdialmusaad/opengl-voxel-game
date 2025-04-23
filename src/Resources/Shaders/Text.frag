#version 420 core

layout (binding = 1) uniform sampler2D textureID;

out vec4 FragColor;
in vec2 TexCoord;
in vec4 TextCol;

void main() 
{
	FragColor = texture(textureID, TexCoord) * TextCol;
}
