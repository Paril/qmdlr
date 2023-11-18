#version 330 core

uniform mat4 u_projection;
uniform mat4 u_modelview;
 
in vec3 i_position;
in vec4 i_color;

out vec4 v_color;
 
void main()
{
	v_color = i_color;
	gl_Position = u_projection * u_modelview * vec4(i_position, 1.0);
}
