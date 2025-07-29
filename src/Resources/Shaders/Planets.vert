#version 430 core

layout (std140, binding = 0) uniform GameMatrices {
	mat4 originMatrix;
	mat4 starMatrix;
	mat4 planetsMatrix;
};

layout (location = 0) in vec4 data; // X, Z, R, B
out vec4 c;

void main() {
	gl_Position = (planetsMatrix * vec4(data.x, sign(data.z), data.y, 1.0)).xyzz;
	c = vec4(abs(data.z), 0.89, data.w, 1.0);
}
