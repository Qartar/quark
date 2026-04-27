// r_font.h
//

#pragma once

#include "cm_color.h"
#include "cm_string.h"
#include "cm_vector.h"

#include "gl/gl_buffer.h"
#include "gl/gl_shader.h"
#include "gl/gl_vertex_array.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

struct font_sdf;
class image;
class shader;
class system;

//------------------------------------------------------------------------------
class font
{
public:
    static void init();

    font(render::system* renderer, string::view name, int size);
    ~font();

    bool compare(string::view name, int size) const;
    void draw(string::view string, vec2 position, color4 color, vec2 scale) const;
    vec2 size(string::view string, vec2 scale) const;

private:
    string::buffer _name;
    int _size;

    std::unique_ptr<font_sdf> _sdf;
    render::image const* _image;

    struct instance {
        float position[2];
        int index;
        uint32_t color;
    };

    friend struct font_sdf;

    struct glyph_info {
        float size[2]; // size of full glyph rect
        float cell[2]; // top-left coordinate of glyph rect
        float offset[2]; // offset of glyph origin relative to cell origin
        float advance[2]; // offset to next character in text
    };

    gl::vertex_array _vao;
    gl::vertex_buffer<instance> _vbo;
    gl::index_buffer<uint16_t> _ibo;
    gl::shader_storage_buffer<glyph_info> _ssbo;
    render::shader const* _shader;

    //! Offset into circular vertex instance buffer, mutable so it can be
    //! modified inside of draw.
    mutable int _instance_offset;

    static constexpr int max_instances = 65536 / sizeof(instance);

private:
    // additional opengl bindings
    using PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE = void (APIENTRY*)(GLenum mode, GLsizei count, GLenum type, void const* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);

    static PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE glDrawElementsInstancedBaseVertexBaseInstance;
};

} // namespace render
