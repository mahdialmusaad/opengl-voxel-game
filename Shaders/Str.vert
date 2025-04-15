#version 420 core

const vec3 colours[4] = { 
	vec3(1.0, 1.0, 1.0), 
	vec3(1.0, 0.8, 0.8), 
	vec3(0.8, 0.8, 1.0), 
	vec3(0.6, 0.6, 1.0) 
};

const float pointSizes[10] = { 
	1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 3.0 
};

layout (location = 0) in int data;
//DDZZ ZZZZ ZZZZ YYYY YYYY YYXX XXXX XXXX

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

out vec4 col;

void main() {
	float x = (data >> 0  & 0x3ff) - 511.5;
	float y = (data >> 10 & 0x3ff) - 511.5;
	float z = (data >> 20 & 0x3ff) - 511.5;
	int appearance = gl_VertexID + (data >> 30);

	gl_Position = starMatrix * vec4(x, y, z, 1.0);
	gl_Position.xyzw = gl_Position.xyzz;
	gl_PointSize = pointSizes[appearance & 0xa];
	col = vec4(colours[appearance & 0x3], starTime);
}