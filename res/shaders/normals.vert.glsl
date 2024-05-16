#version 330 core

#include "ubo.glsl"

uniform mat4 u_projection;
uniform mat4 u_modelview;
 
in vec3 i_position;
in vec3 i_normal;
in int i_selected;

out vec4 v_color;
 
void main()
{
	vec4 p = vec4(i_position, 1.0);
	vec3 n = i_normal;

	if ((flags & FLAG_DRAG_SELECTED) != 0 && i_selected != 0)
	{
		p = drag3DMatrix * p;
		n = normalize(mat3(drag3DMatrix) * n);
	}

	v_color = vec4((n * 0.5) + vec3(0.5), 1.0);
	gl_Position = u_projection * u_modelview * p;
}
