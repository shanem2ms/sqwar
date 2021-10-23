$input v_texcoord0, v_normal
/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "uniforms.sh"
#include <bgfx_shader.sh>
SAMPLER2D(s_depth, 0);
SAMPLER2D(s_vid, 1);

void main()
{
	float xv = v_texcoord0.x * (1 - v_texcoord0.x);
	float yv = v_texcoord0.y * (1 - v_texcoord0.y);
	vec3 lightdir = vec3(0, 1, 0.5);
	normalize(lightdir);
	float diffuse = abs(dot(lightdir, v_normal));
	//if (xv > 0.02 && yv > 0.02)
	//	discard;
	//vec4 vd = texture2D(s_vid, v_texcoord0.yx);
	gl_FragColor.r = texture2D(s_depth, v_texcoord0.yx);
	gl_FragColor.gb = vec2(0.2,0.2);

	gl_FragColor.a = 1;
} 
