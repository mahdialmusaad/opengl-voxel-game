#version 420 core

layout (location = 0) in uint data;
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
	gl_Position = (starMatrix * vec4(
		(data & 0x3ff) - 511.5, 
		(data >> 10 & 0x3ff) - 511.5, 
		(data >> 20 & 0x3ff) - 511.5, 
		1.0
	)).xyzz;

	const vec3 colours[4] = { 
		vec3(1.0, 1.0, 1.0),
		vec3(1.0, 0.8, 0.8),
		vec3(0.8, 0.8, 1.0),
		vec3(0.8, 0.8, 1.0)
	};

	const uint style = data >> 30;
	gl_PointSize = float(style) * 0.67;
	col = vec4(colours[(style + gl_VertexID) & 3], starTime);
}