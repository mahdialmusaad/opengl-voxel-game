#version 430 core
layout (location = 0) in vec4 data; // X, Y, Z, C

layout (std140, binding = 0) uniform GameMatrices {
	mat4 matrix;
	mat4 originMatrix;
	mat4 starMatrix;
	mat4 sunMoonMatrix;
};

out vec4 c;
const vec4 pData[4] = { 
	// Size, { R, G, B }
	vec4(150.0,	 0.92, 0.78, 0.23),
	vec4(130.0,	 0.89, 0.85, 0.77),
	vec4(50.0,	 0.59, 0.81, 0.94),
	vec4(40.0,	 0.78, 0.95, 1.00)
};

void main() {
	gl_Position = (sunMoonMatrix * vec4(data.xyz, 1.0)).xyzz;
	const vec4 pointData = pData[int(data.w)];
	gl_PointSize = pointData.x;
	c = vec4(pointData.yzw, 1.0);
}
