layout(location = 0) in vec3 position;

out vec4 c;

void main()
{
	gl_Position = (M4_planets * vec4(position, 0.0)).xyzz;
	c = vec4(1.0, 0.0, 0.0, 1.0);
}

@

out vec4 f;
in vec4 c;

void main()
{
	f = c;
}