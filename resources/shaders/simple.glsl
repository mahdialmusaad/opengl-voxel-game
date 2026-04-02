layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 rgb;

out vec4 c;

void main()
{
	gl_Position = M4_camera * vec4(pos, 1.0);
	c = vec4(rgb, 1.0);
	gl_PointSize = 10.0;
}

@

in vec4 c;
out vec4 f;

void main()
{
	f = c;
}