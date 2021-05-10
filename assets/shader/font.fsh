#version 420

layout(binding = 0) uniform sampler2D sdf;

in vec2 vtx_texcoord;
in vec4 vtx_color;

out vec4 frag_color;

float median(vec3 v) {
    float vmin = min(v.x, min(v.y, v.z));
    float vmax = max(v.x, max(v.y, v.z));
    return dot(v, vec3(1.0)) - vmin - vmax;
}

void main() {
    vec2 uv = vtx_texcoord / vec2(textureSize(sdf, 0));
    float f = median(texture(sdf, uv).xyz);
    float g = fwidth(f);
    float d = smoothstep(-0.5 * g, 0.5 * g, 0.5 - f);
    frag_color = vec4(1,1,1,d) * vtx_color;
}
