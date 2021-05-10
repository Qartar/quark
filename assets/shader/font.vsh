#version 430 compatibility

struct glyph_info {
    vec2 size;
    vec2 cell;
    vec2 offset;
    vec2 advance;
};

layout(std430, binding = 0) buffer ssbo_glyphs {
    readonly glyph_info glyphs[];
};

layout(location = 0) in vec2 in_position;
layout(location = 1) in int in_index;
layout(location = 2) in vec4 in_color;

out vec2 vtx_texcoord;
out vec4 vtx_color;

void main() {
    vec2 size = glyphs[in_index].size;
    vec2 cell = glyphs[in_index].cell;
    vec2 offset = glyphs[in_index].offset;

    vec2 flip = vec2(1, gl_ProjectionMatrix[1][1] < 0 ? -1 : 1);

    vec4 v0 = vec4(in_position + (offset       ) * flip, 0.0, 1.0);
    vec4 v1 = vec4(in_position + (offset + size) * flip, 0.0, 1.0);
    gl_Position = gl_ModelViewProjectionMatrix * 
                  vec4((gl_VertexID & 1) == 1 ? v1.x : v0.x,
                       (gl_VertexID & 2) == 2 ? v1.y : v0.y, 0.0, 1.0);

    vec2 u0 = cell;
    vec2 u1 = cell + size;
    vtx_texcoord = vec2((gl_VertexID & 1) == 1 ? u1.x : u0.x,
                        (gl_VertexID & 2) == 2 ? u1.y : u0.y);

    vtx_color = in_color;
}
