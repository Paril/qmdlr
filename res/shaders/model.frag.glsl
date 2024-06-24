#version 330 core
 
precision mediump float;

#include "ubo.glsl"

uniform sampler2D u_texture;
uniform bool u_shaded; // light affects polygonal rendering (3d only)
uniform bool u_2d; // use 2D color set
uniform int u_line; // is wireframe (1), or is overlay enabled (2)
uniform vec4 u_face3D[2];
uniform vec4 u_face2D[2];
uniform vec4 u_line3D[2];
uniform vec4 u_line2D[2];

in vec2 v_texcoord;
in vec3 v_normal;
flat in int v_selected;
in vec2 v_bary;

out vec4 o_color;

#define NUM_LIGHTS 3

vec3 lights[] = vec3[NUM_LIGHTS] (
	vec3(0.57735,-0.57735,-0.57735),
	vec3(-0.6666,-0.3333,-0.6666),
	vec3(0.08909,0.445435,0.89087)
);

float gridFactor (vec2 vBC, float width, float feather)
{
	float w1 = width - feather * 0.5;
	vec3 bary = vec3(vBC.x, vBC.y, 1.0 - vBC.x - vBC.y);
	vec3 d = fwidth(bary);
	vec3 a3 = smoothstep(d * w1, d * (w1 + feather), bary);
	return min(min(a3.x, a3.y), a3.z);
}

float gridFactor (vec2 vBC, float width)
{
	vec3 bary = vec3(vBC.x, vBC.y, 1.0 - vBC.x - vBC.y);
	vec3 d = fwidth(bary);
	vec3 a3 = smoothstep(d * (width - 0.5), d * (width + 0.5), bary);
	return min(min(a3.x, a3.y), a3.z);
}

void main()
{
	o_color = texture2D(u_texture, v_texcoord);
	
	if (u_shaded)
	{
		float light = min(1.0, dot(lights[0], v_normal) + dot(lights[1], v_normal) + dot(lights[2], v_normal));
		o_color.rgb *= vec3(0.75 + (light * 0.25));
	}

	vec4 additional_color;

	if (u_2d)
	{
		if (u_line == 1)
			additional_color = u_line2D[v_selected];
		else
			additional_color = u_face2D[v_selected];
	}
	else
	{
		if (u_line == 1)
			additional_color = u_line3D[v_selected];
		else
			additional_color = u_face3D[v_selected];
	}

	if ((flags & FLAG_FACE_MODE) != 0)
	{
		if (u_line == 1)
			o_color = additional_color;
		else
			o_color += additional_color;

		vec4 line_color = u_2d ? u_line2D[v_selected] : u_line3D[v_selected];
		o_color.rgb = mix(line_color.rgb, o_color.rgb, gridFactor(v_bary, 0.5) * line_color.a);
	}
	else if (u_line == 2)
	{
		o_color.rgb = mix(vec3(1), o_color.rgb, gridFactor(v_bary, 0.5));
	}
}