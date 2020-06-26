#version 420

layout(binding = 0) uniform sampler2D lightmap;

void main()
{
    ivec2 uv = ivec2(gl_FragCoord.xy) * 2;
    ivec2 sz = textureSize(lightmap, 0);

    float xmask = uv.x + 1 < sz.x ? 1.0 : 1.0;
    float ymask = uv.y + 1 < sz.y ? 1.0 : 1.0;
    vec4 mask = vec4(1.0, xmask, ymask, xmask * ymask);

    mat4 samples = mat4(
        texelFetch(lightmap, uv + ivec2(    0,     0), 0),
        texelFetch(lightmap, uv + ivec2(xmask,     0), 0),
        texelFetch(lightmap, uv + ivec2(    0, ymask), 0),
        texelFetch(lightmap, uv + ivec2(xmask, ymask), 0)
    );

    gl_FragColor = samples * mask;
}
