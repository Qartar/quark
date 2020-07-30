#version 420

layout(binding = 0) uniform sampler2D lightmap;
layout(binding = 1) uniform sampler2D barycenter;

layout(location = 0) out vec4 out_lightmap;
layout(location = 1) out vec2 out_barycenter;

void main()
{
    ivec2 uv = ivec2(gl_FragCoord.xy) * 2;
    ivec2 sz = textureSize(lightmap, 0);

    float xmask = uv.x + 1 < sz.x ? 1.0 : 1.0;
    float ymask = uv.y + 1 < sz.y ? 1.0 : 1.0;
    vec4 mask = vec4(1.0, xmask, ymask, xmask * ymask);

    mat4 samples = mat4(
        texelFetch(lightmap, uv + ivec2(0, 0), 0),
        texelFetch(lightmap, uv + ivec2(1, 0), 0),
        texelFetch(lightmap, uv + ivec2(0, 1), 0),
        texelFetch(lightmap, uv + ivec2(1, 1), 0)
    );

    mat4x2 centers = mat4x2(
        texelFetch(barycenter, uv + ivec2(0, 0), 0).xy,
        texelFetch(barycenter, uv + ivec2(1, 0), 0).xy,
        texelFetch(barycenter, uv + ivec2(0, 1), 0).xy,
        texelFetch(barycenter, uv + ivec2(1, 1), 0).xy
    );

    vec4 lum = vec4(0.2126, 0.7152, 0.0722, 0) * samples + vec4(1e-8);
    float lsum = dot(lum, vec4(1,1,1,1));

    out_lightmap = samples * mask;
    out_barycenter = centers * lum / lsum;
    //out_barycenter = centers * vec4(1,1,1,1) * 0.25;
}
