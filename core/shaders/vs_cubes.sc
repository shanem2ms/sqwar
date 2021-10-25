$input a_position, a_texcoord0, a_normal, i_data0
$output v_texcoord0, v_normal, v_vtxcolor


/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */ 


#include <bgfx_shader.sh> 

vec3 unpackColor(float f) {
    vec3 color;
    color.b = floor(f / 256.0 / 256.0);
    color.g = floor((f - color.b * 256.0 * 256.0) / 256.0);
    color.r = floor(f - color.b * 256.0 * 256.0 - color.g * 256.0);
    // now we have a vec3 with the 3 components in range [0..255]. Let's normalize it!
    return color / 255.0;
}

void main()
{ 
	vec2 tx = a_texcoord0;
	v_texcoord0 = tx; 
	v_normal = a_normal;  
	float s = 0.001;
	vec3 col = unpackColor(i_data0.w).bgr;
	v_vtxcolor = vec4(col, 1);
	gl_Position = mul(u_modelViewProj, vec4(a_position.x * s + i_data0.x, a_position.y * s + i_data0.y, a_position.z * s + i_data0.z, 1.0) );
}
