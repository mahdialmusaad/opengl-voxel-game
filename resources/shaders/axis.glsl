layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 rgb;

out vec4 c;

void main()
{
	vec3 scale = vec3(0.03 * FLT_aspect);
	vec2 offset = vec2(0.05, 0.05 * FLT_aspect) + vec2(-1.0);
	gl_Position = vec4(offset + (M4_axis * vec4(pos * scale, 0.0)).xy, 0.0, 1.0);
	c = vec4(rgb, 1.0);
}

@

in vec4 c;
out vec4 f;

void main()
{
	f = c;
}