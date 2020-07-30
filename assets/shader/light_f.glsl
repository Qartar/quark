#version 420

const int lods = 13;

layout(binding = 0) uniform sampler2D lightmap;
layout(binding = 1) uniform sampler2D barycenter;

vec4 illuminance(vec2 pos, ivec3 texel)
{
    vec2 light_pos = texelFetch(barycenter, texel.xy, texel.z).xy;
    vec2 dir = pos - light_pos;
    vec4 light = texelFetch(lightmap, texel.xy, texel.z);
    return 0.1 * light / dot(dir, dir);
}

vec4 luminance(ivec2 pos)
{
    vec2 world_pos = vec2(pos);// + vec2(0.5);

    vec4 total = vec4(0);
    vec4 prev = vec4(0);

    ivec3 texel = ivec3(0, 0, lods - 1);

    for (int ii = lods; ii > 4; --ii) {
        vec4 L00 = illuminance(world_pos, texel + ivec3(0,0,-1));
        vec4 L01 = illuminance(world_pos, texel + ivec3(0,1,-1));
        vec4 L10 = illuminance(world_pos, texel + ivec3(1,0,-1));
        vec4 L11 = illuminance(world_pos, texel + ivec3(1,1,-1));

        vec4 lum = vec4(0.2126, 0.7152, 0.0722, 0) * mat4(L00, L01, L10, L11);
        float lmax = max(lum.x, max(lum.y, max(lum.z, lum.w)));

        //vec4 lum = vec4(0,0,1,0);
        //float lmax = 1;

        //vec4 lsum = L00 + L01 + L10 + L11;
        //if (dot(vec4(0.2126, 0.7152, 0.0722, 0), lsum - prev) < 0.0) {
        //    return total;
        //}

        total -= prev;
        total += L00 + L01 + L10 + L11;

        if (lmax == lum.x) {
            texel += ivec3(0,0,-1);
            prev = L00;
        } else if (lmax == lum.y) {
            texel += ivec3(0,1,-1);
            prev = L01;
        } else if (lmax == lum.z) {
            texel += ivec3(1,0,-1);
            prev = L10;
        } else {
            texel += ivec3(1,1,-1);
            prev = L11;
        }

        texel.xy *= 2;
    }

    return total;
}

void main()
{
    gl_FragColor = luminance(ivec2(gl_FragCoord.xy));
}
