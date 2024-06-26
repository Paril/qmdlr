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
flat in int v_selected_flags;
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
	int selectedFace = ((v_selected_flags & FLAG_SELECTED_FACE) != 0) ? 1 : 0;

	if (u_2d)
	{
		if (u_line == 1)
			additional_color = u_line2D[selectedFace];
		else
			additional_color = u_face2D[selectedFace];
	}
	else
	{
		if (u_line == 1)
			additional_color = u_line3D[selectedFace];
		else
			additional_color = u_face3D[selectedFace];
	}

	vec4 line_color = u_2d ? u_line2D[selectedFace] : u_line3D[selectedFace];

	if (u_line == 1)
		o_color = line_color;
	else
	{
		float grid = gridFactor(v_bary, 0.75, 0.5);

		if ((flags & FLAG_FACE_MODE) != 0)
			o_color += additional_color;

		vec3 line_mixed_color = mix(o_color.rgb, line_color.rgb, line_color.a);

		o_color.rgb = mix(line_mixed_color, o_color.rgb, grid);
	}
}