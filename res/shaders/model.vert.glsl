#version 330 core

uniform mat4 u_projection;
uniform mat4 u_modelview;
 
in vec3 i_position;
in vec2 i_texcoord;
in vec4 i_color;
in vec3 i_normal;

out vec4 v_color;
out vec2 v_texcoord;
out vec3 v_normal;
 
void main()
{
	v_color = i_color;
	v_texcoord = i_texcoord;
	v_normal = i_normal;
	gl_Position = u_projection * u_modelview * vec4(i_position, 1.0);
}
