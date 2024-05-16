#version 330 core

#include "ubo.glsl"

uniform mat4 u_projection;
uniform mat4 u_modelview;
 
in vec3 i_position;
in vec4 i_color;
in int i_selected;

out vec4 v_color;
 
void main()
{
	vec4 p = vec4(i_position, 1.0);

	if ((flags & FLAG_DRAG_SELECTED) != 0 && i_selected != 0)
		p = drag3DMatrix * p;

	v_color = i_color;
	gl_Position = u_projection * u_modelview * p;
}
