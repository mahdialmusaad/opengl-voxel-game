#version 430 core
layout(binding=1)uniform sampler2D i;out vec4 o;in vec2 t;in vec4 c;void main(){o=texture(i,t)*c;}