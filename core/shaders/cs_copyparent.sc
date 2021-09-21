#include <bgfx_compute.sh>
#include "uniforms.sh"
#include "noise.sh"

SAMPLER2D(s_texColor, 0);
IMAGE2D_WR(s_target, rgba16f, 1);

float noise_octaves(vec2 p)
{
    float val = 0;
    float scale = 1.0;
    float mul[5] = { 1.89, .89, 2.13, 2.17, 1.83 };
    for (int i = 0; i < 10; ++i)
    {
        val += snoise(p) * scale;
        p *= 1.89;
        scale *= 0.5;
    }

    return val;
}


NUM_THREADS(16, 16, 1)
void main()
{    
    vec2 uv = (gl_GlobalInvocationID.xy - vec2(64.0, 64.0)) / 256.0;
    vec2 offset = uv * u_params[0].x + u_params[0].zw;
    float val = noise_octaves(offset) * u_params[0].y;

    vec2 v = gl_GlobalInvocationID.xy * v_params[0].xy + v_params[0].zw;

    v /= vec2(384, 384);
    v += vec2(0.5 / 384, 0.5 / 384);

    float parentval = texture2DLod(s_texColor, v.xy, 0);
	imageStore(s_target, ivec2(gl_GlobalInvocationID.xy), vec4(0, 0, 0, parentval + val + 1));
}


