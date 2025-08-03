#version 330 core
out vec4 out_color;
in vec2 a_uv;
in vec4 a_color;

uniform sampler2D u_tex;

void main()
{
	out_color = vec4(1, 1, 1, 1);
	out_color = vec4(a_uv.x, a_uv.y, 0, 1);
	out_color = texture(u_tex, a_uv) * a_color;
}