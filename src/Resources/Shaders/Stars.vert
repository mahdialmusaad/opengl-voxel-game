#version 420 core
layout (location = 0) in vec4 data; // X, Y, Z, P

layout (std140, binding = 0) uniform GameMatrices {
	mat4 matrix;
	mat4 originMatrix;
	mat4 starMatrix;
};

layout (std140, binding = 1) uniform GameTimes {
	float time;
	float fogTime;
	float starTime;
	float gameTime;
	float cloudsTime;
};

out vec4 c;
const vec3 colours[4] = { 
	vec3(1.0, 1.0, 1.0),
	vec3(1.0, 0.8, 0.8),
	vec3(0.8, 0.8, 1.0),
	vec3(0.8, 0.8, 1.0)
};

void main() {
	gl_Position = (starMatrix * vec4(data.xyz, 1.0)).xyzz;
	gl_PointSize = data.w;
	c = vec4(colours[int(data.w * gl_VertexID) & 3], starTime);
}