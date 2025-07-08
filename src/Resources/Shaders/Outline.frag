#version 430 core

uniform float colour = 0.0;
out vec4 FragColor;

void main()
{
	FragColor = vec4(vec3(colour), 1.0);
}
