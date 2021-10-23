$input a_position, a_texcoord0, a_normal, i_data0
$output v_texcoord0, v_normal


/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */ 


#include <bgfx_shader.sh> 

void main()
{ 
	vec2 tx = a_texcoord0;
	v_texcoord0 = tx; 
	v_normal = a_normal;  
	float s = 0.001;
	gl_Position = mul(u_modelViewProj, vec4(a_position.x * s + i_data0.x, a_position.y * s + i_data0.y, a_position.z * s + i_data0.z, 1.0) );
}
