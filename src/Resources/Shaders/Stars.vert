#version 430 core
layout (location = 0) in vec3 data;

layout (std140, binding = 0) uniform GameMatrices {
	mat4 originMatrix;
	mat4 starMatrix;
	mat4 planetsMatrix;
};

layout (std140, binding = 1) uniform GameTimes {
	float time;
	float fogEnd;
	float fogRange;
	float starTime;
	float gameTime;
	float cloudsTime;
};

uniform const vec3 colourData[16] = {
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 1.0, 0.6 },
	{ 1.0, 1.0, 0.6 },
	{ 1.0, 1.0, 0.6 },
	{ 1.0, 0.6, 0.6 },
	{ 0.6, 0.6, 1.0 }
};

out vec4 c;

void main() {
	c = vec4(colourData[gl_VertexID & 15], starTime);
	gl_PointSize = 1.0 + fract(length(data * gl_VertexID)) * 1.5;
	gl_Position = (starMatrix * vec4(data.xyz + vec3(gl_VertexID / gl_PointSize) * 1e-5, 0.0)).xyzz;
}