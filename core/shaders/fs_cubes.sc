$input v_texcoord0, v_normal

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "uniforms.sh"
#include <bgfx_shader.sh>

void main()
{
	float xv = v_texcoord0.x * (1 - v_texcoord0.x);
	float yv = v_texcoord0.y * (1 - v_texcoord0.y);
	vec3 lightdir = vec3(1, 1, 0);
	normalize(lightdir);
	float diffuse = abs(dot(lightdir, v_normal));
	//if (xv > 0.02 && yv > 0.02)
	//	discard;
	gl_FragColor.rgb = vec3(u_params[0].xyz) * (diffuse * 0.8 + 0.2);
	gl_FragColor.a = 1;
} 
