#include <bgfx_compute.sh>
#include "uniforms.sh"

SAMPLER2D(s_texColor, 0);
IMAGE2D_WR(s_target, r16f, 1);


NUM_THREADS(16, 16, 1)
void main()
{    
    vec4 outpixel = texelFetch(s_texColor,gl_GlobalInvocationID.xy,0);  
	imageStore(s_target, ivec2(gl_GlobalInvocationID.xy), outpixel.aaaa - vec4(1,1,1,1));
}
