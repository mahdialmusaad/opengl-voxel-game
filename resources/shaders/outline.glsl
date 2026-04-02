layout(location = 0) in vec3 position;

void main()
{
	gl_Position = M4_origin * vec4(position - V4_raycast_lpos.xyz, 1.0);
}

@

out vec4 f;

void main()
{
	f = vec4(0.0, 0.0, 0.0, 1.0);
}