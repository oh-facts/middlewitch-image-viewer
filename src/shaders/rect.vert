#version 330 core
layout (location = 0) in vec2 l_pos;
layout (location = 1) in vec2 l_uv;
layout (location = 2) in vec4 l_color;

out vec2 a_uv;
out vec4 a_color;

uniform vec2 u_screen_size;

void main()
{
	a_uv = l_uv;
	a_color = l_color;
	
	vec2 pos = l_pos;
	pos.y = u_screen_size.y - pos.y;
	
	gl_Position = vec4((pos / u_screen_size) * 2.0 - 1.0, 0.0, 1.0);
}