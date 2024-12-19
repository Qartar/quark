//  shaders/sm4/bloom_upsample.frag
//

#version 430

//  input
layout(binding = 0) uniform sampler2D inColor;
layout(location = 0) uniform float kernelRadius;

//  output
layout(location = 0) out vec4 outColor;

//------------------------------------------------------------------------------
void main()
{
    // texel size in the input image
    vec2 texelSize = vec2(1.0) / vec2(textureSize(inColor, 0));
    // texel position in the input image
    vec2 texelCoord = 0.5 * gl_FragCoord.xy * texelSize;
    // kernel size in texels of the input image
    vec2 kernelSize = kernelRadius * texelSize / texelSize.yy;

    //
    //  Perform 9-tap filter as described in Jimenez(2014)
    //

    const vec2 kernelOffset[9] = {
        vec2(-1., -1.), vec2( 0., -1.), vec2( 1., -1.), //  0  1  2
        vec2(-1.,  0.), vec2( 0.,  0.), vec2( 1.,  0.), //  3  4  5
        vec2(-1.,  1.), vec2( 0.,  1.), vec2( 1.,  1.)  //  6  7  8
    };

    vec4 tap[9];

    // Fetch bilinear samples
    for (int ii = 0; ii < 9; ++ii) {
        vec2 pos = texelCoord + kernelOffset[ii] * kernelSize;
        tap[ii] = textureLod(inColor, pos, 0);
    }

    // Weight and sum
    outColor = (1. / 16.) * tap[0] + (2. / 16.) * tap[1] + (1. / 16.) * tap[2]
             + (2. / 16.) * tap[3] + (4. / 16.) * tap[4] + (2. / 16.) * tap[5]
             + (1. / 16.) * tap[6] + (2. / 16.) * tap[7] + (1. / 16.) * tap[8];
}
