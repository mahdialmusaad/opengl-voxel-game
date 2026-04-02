/* Format: { X, Y, Z, 1 }, { R, G, B, A }. */
layout(location = 0) in mat2x4 data;

out vec4 c;

void main()
{
	gl_Position = (M4_planets * data[0]).xyzz; 
	c = data[1];
}

@

out vec4 f;
in vec4 c;

void main()
{
	f = c;
}