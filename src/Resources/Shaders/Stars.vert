#version 420 core
layout (location = 0) in vec4 data;

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

uniform const vec3 colourData[4] = {
	{ 1.0, 0.8, 1.0 },
	{ 1.0, 1.0, 0.8 },
	{ 0.8, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 }
};

out vec4 c;

void main() {
	gl_Position = (starMatrix * vec4(data.xyz, 1.0)).xyzz;
	c = vec4(colourData[gl_VertexID & 3], starTime);
	gl_PointSize = data.w;
}