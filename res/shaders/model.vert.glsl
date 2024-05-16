#version 330 core

#include "ubo.glsl"

uniform mat4 u_projection;
uniform mat4 u_modelview;
 
in vec3 i_position;
in vec2 i_texcoord;
in vec3 i_normal;
in int i_selected;
in int i_selected_vertex;

out vec2 v_texcoord;
out vec3 v_normal;
flat out int v_selected;
 
void main()
{
	vec4 p = vec4(i_position, 1.0);
	vec3 n = i_normal;
	vec2 t = i_texcoord;

	if ((flags & FLAG_DRAG_SELECTED) != 0 && i_selected_vertex != 0)
	{
		p = drag3DMatrix * p;
		n = normalize(mat3(drag3DMatrix) * n);
	}

	if ((flags & FLAG_UV_SELECTED) != 0 && v_selected != 0)
		t = (vec4(t, 0, 1) * dragUVMatrix).xy;

	v_texcoord = t;
	v_normal = n;
    v_selected = i_selected;
	gl_Position = u_projection * u_modelview * p;
}
