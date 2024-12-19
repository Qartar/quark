//  shaders/sm4/bloom_downsample.frag
//

#version 430

//  input
layout(binding = 0) uniform sampler2D inColor;
layout(location = 0) uniform float base_luma;

//  output
layout(location = 0) out vec4 outColor;

//------------------------------------------------------------------------------
vec4 box_filter_luma(in vec4 a, in vec4 b, in vec4 c, in vec4 d)
{
    // Conversion factors from RGB to luma
    const vec4 luma = vec4(0.2126, 0.7152, 0.0722, 0.);
    // Place input colors into matrix columns
    mat4 color = mat4(a, b, c, d);
    // Calculate weights for each column (color)
    vec4 weight = vec4(base_luma) / (vec4(base_luma) + luma * color);
    // Apply weights to each column and return weighted average
    return color * weight / dot(vec4(1.), weight);
}

//------------------------------------------------------------------------------
vec4 box_filter(in vec4 a, in vec4 b, in vec4 c, in vec4 d)
{
    // Place input colors into matrix columns
    mat4 color = mat4(a, b, c, d);
    // Return weighted average
    return color * vec4(0.25);
}

//------------------------------------------------------------------------------
void main()
{
#if 1
    // texel size in the input image
    vec2 texelSize = vec2(1.0) / vec2(textureSize(inColor, 0));
    // texel position in the input image
    vec2 texelCoord = 2.0 * gl_FragCoord.xy * texelSize;

    //
    //  Perform 13-tap filter as described in Jimenez(2014)
    //

    const vec2 texelOffset[13] = {
        vec2(-2., -2.), vec2( 0., -2.), vec2( 2., -2.), //  0   1   2
                vec2(-1., -1.), vec2( 1., -1.),         //    3   4
        vec2(-2.,  0.), vec2( 0.,  0.), vec2( 2.,  0.), //  5   6   7
                vec2(-1.,  1.), vec2( 1.,  1.),         //    8   9
        vec2(-2.,  2.), vec2( 0.,  2.), vec2( 2.,  2.)  // 10  11  12
    };

    vec4 tap[13];

    // Fetch bilinear samples
    for (int ii = 0; ii < 13; ++ii) {
        vec2 pos = texelCoord + texelOffset[ii] * texelSize;
        tap[ii] = textureLod(inColor, pos, 0);
    }

    // Weight and sum
#if 0
    if (base_luma > 0.) {
        outColor = 0.125 * box_filter_luma(tap[ 0], tap[ 1], tap[ 5], tap[ 6])
                 + 0.125 * box_filter_luma(tap[ 1], tap[ 2], tap[ 6], tap[ 7])
                 + 0.500 * box_filter_luma(tap[ 3], tap[ 4], tap[ 8], tap[ 9])
                 + 0.125 * box_filter_luma(tap[ 5], tap[ 6], tap[10], tap[11])
                 + 0.125 * box_filter_luma(tap[ 6], tap[ 7], tap[11], tap[12]);
    } else {
        outColor = 0.125 * box_filter(tap[ 0], tap[ 1], tap[ 5], tap[ 6])
                 + 0.125 * box_filter(tap[ 1], tap[ 2], tap[ 6], tap[ 7])
                 + 0.500 * box_filter(tap[ 3], tap[ 4], tap[ 8], tap[ 9])
                 + 0.125 * box_filter(tap[ 5], tap[ 6], tap[10], tap[11])
                 + 0.125 * box_filter(tap[ 6], tap[ 7], tap[11], tap[12]);
    }
#else
    outColor = 0.125 * box_filter(tap[ 0], tap[ 1], tap[ 5], tap[ 6])
             + 0.125 * box_filter(tap[ 1], tap[ 2], tap[ 6], tap[ 7])
             + 0.500 * box_filter(tap[ 3], tap[ 4], tap[ 8], tap[ 9])
             + 0.125 * box_filter(tap[ 5], tap[ 6], tap[10], tap[11])
             + 0.125 * box_filter(tap[ 6], tap[ 7], tap[11], tap[12]);
#endif

#elif 1
    outColor = texture2D(inColor, gl_FragCoord.xy / vec2(textureSize(inColor, 0)));
#else
    outColor = vec4(1,0,0,1);
#endif
}
