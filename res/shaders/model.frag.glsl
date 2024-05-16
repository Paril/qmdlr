#version 330 core
 
precision mediump float;

#include "ubo.glsl"

uniform sampler2D u_texture;
uniform bool u_shaded; // light affects polygonal rendering (3d only)
uniform bool u_2d, u_line; // use 2D color set
uniform vec4 u_face3D[2];
uniform vec4 u_face2D[2];
uniform vec4 u_line3D[2];
uniform vec4 u_line2D[2];

in vec2 v_texcoord;
in vec3 v_normal;
flat in int v_selected;

out vec4 o_color;

#define NUM_LIGHTS 3

vec3 lights[] = vec3[NUM_LIGHTS] (
	vec3(0.57735,-0.57735,-0.57735),
	vec3(-0.6666,-0.3333,-0.6666),
	vec3(0.08909,0.445435,0.89087)
);

void main()
{
	o_color = texture2D(u_texture, v_texcoord);
	
	if (u_shaded)
	{
		float light = min(1.0, dot(lights[0], v_normal) + dot(lights[1], v_normal) + dot(lights[2], v_normal));
		o_color.rgb *= vec3(0.75 + (light * 0.25));
	}

	if ((flags & FLAG_FACE_MODE) != 0)
	{
		vec4 additional_color;

		if (u_2d)
		{
			if (u_line)
				additional_color = u_line2D[v_selected];
			else
				additional_color = u_face2D[v_selected];
		}
		else
		{
			if (u_line)
				additional_color = u_line3D[v_selected];
			else
				additional_color = u_face3D[v_selected];
		}

		if (u_line)
			o_color = additional_color;
		else
			o_color += additional_color;
	}
}