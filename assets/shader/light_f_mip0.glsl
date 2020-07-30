#version 420

layout(binding = 0) uniform sampler2D lightmap;

layout(location = 0) out vec4 out_lightmap;
layout(location = 1) out vec2 out_barycenter;

void main()
{
    ivec2 uv = ivec2(gl_FragCoord.xy);

    //out_lightmap = vec4(0.1);
    out_lightmap = texelFetch(lightmap, uv.xy, 0);
    out_barycenter = vec2(uv);// + vec2(0.5, 0.5);
}
