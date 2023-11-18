#version 330 core 
 
precision mediump float;

uniform sampler2D u_texture;
uniform bool u_shaded;

in vec4 v_color;
in vec2 v_texcoord;
in vec3 v_normal;

out vec4 o_color;

#define NUM_LIGHTS 3

vec3 lights[] = vec3[NUM_LIGHTS] (
	vec3(0.57735,-0.57735,-0.57735),
	vec3(-0.6666,-0.3333,-0.6666),
	vec3(0.08909,0.445435,0.89087)
);

void main()
{
	o_color = texture2D(u_texture, v_texcoord) * v_color;
	
	if (u_shaded)
	{
		float light = min(1.0, dot(lights[0], v_normal) + dot(lights[1], v_normal) + dot(lights[2], v_normal));
		o_color.rgb *= vec3(0.75 + (light * 0.25));
	}
}