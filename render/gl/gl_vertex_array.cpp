// gl_vertex_array.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "gl/gl_vertex_array.h"
#include "gl/gl_buffer.h"
#include "gl/gl_include.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

// OpenGL 1.5
vertex_array::PFNGLBINDBUFFER vertex_array::glBindBuffer = nullptr;
// OpenGL 2.0
vertex_array::PFNGLDISABLEVERTEXATTRIBARRAY vertex_array::glDisableVertexAttribArray = nullptr;
vertex_array::PFNGLENABLEVERTEXATTRIBARRAY vertex_array::glEnableVertexAttribArray = nullptr;
// OpenGL 3.0
vertex_array::PFNGLBINDVERTEXARRAY vertex_array::glBindVertexArray = nullptr;
vertex_array::PFNGLDELETEVERTEXARRAYS vertex_array::glDeleteVertexArrays = nullptr;
vertex_array::PFNGLGENVERTEXARRAYS vertex_array::glGenVertexArrays = nullptr;
// GL_ARB_vertex_attrib_binding
vertex_array::PFNGLBINDVERTEXBUFFER vertex_array::glBindVertexBuffer = nullptr;
vertex_array::PFNGLVERTEXATTRIBFORMAT vertex_array::glVertexAttribFormat = nullptr;
vertex_array::PFNGLVERTEXATTRIBIFORMAT vertex_array::glVertexAttribIFormat = nullptr;
vertex_array::PFNGLVERTEXATTRIBLFORMAT vertex_array::glVertexAttribLFormat = nullptr;
vertex_array::PFNGLVERTEXATTRIBBINDING vertex_array::glVertexAttribBinding = nullptr;
vertex_array::PFNGLVERTEXBINDINGDIVISOR vertex_array::glVertexBindingDivisor = nullptr;
// GL_ARB_direct_state_access
vertex_array::PFNGLCREATEVERTEXARRAYS vertex_array::glCreateVertexArrays = nullptr;
vertex_array::PFNGLDISABLEVERTEXARRAYATTRIB vertex_array::glDisableVertexArrayAttrib = nullptr;
vertex_array::PFNGLENABLEVERTEXARRAYATTRIB vertex_array::glEnableVertexArrayAttrib = nullptr;
vertex_array::PFNGLVERTEXARRAYELEMENTBUFFER vertex_array::glVertexArrayElementBuffer = nullptr;
vertex_array::PFNGLVERTEXARRAYVERTEXBUFFER vertex_array::glVertexArrayVertexBuffer = nullptr;
vertex_array::PFNGLVERTEXARRAYVERTEXBUFFERS vertex_array::glVertexArrayVertexBuffers = nullptr;
vertex_array::PFNGLVERTEXARRAYATTRIBFORMAT vertex_array::glVertexArrayAttribFormat = nullptr;
vertex_array::PFNGLVERTEXARRAYATTRIBIFORMAT vertex_array::glVertexArrayAttribIFormat = nullptr;
vertex_array::PFNGLVERTEXARRAYATTRIBLFORMAT vertex_array::glVertexArrayAttribLFormat = nullptr;
vertex_array::PFNGLVERTEXARRAYATTRIBBINDING vertex_array::glVertexArrayAttribBinding = nullptr;
vertex_array::PFNGLVERTEXARRAYBINDINGDIVISOR vertex_array::glVertexArrayBindingDivisor = nullptr;

//------------------------------------------------------------------------------
void vertex_array::init()
{
    // OpenGL 1.5
    glBindBuffer = (PFNGLBINDBUFFER)wglGetProcAddress("glBindBuffer");
    // OpenGL 2.0
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAY)wglGetProcAddress("glDisableVertexAttribArray");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAY)wglGetProcAddress("glEnableVertexAttribArray");
    // OpenGL 3.0
    glBindVertexArray = (PFNGLBINDVERTEXARRAY)wglGetProcAddress("glBindVertexArray");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYS)wglGetProcAddress("glDeleteVertexArrays");
    glGenVertexArrays = (PFNGLGENVERTEXARRAYS)wglGetProcAddress("glGenVertexArrays");
    // GL_ARB_vertex_attrib_binding
    glBindVertexBuffer = (PFNGLBINDVERTEXBUFFER)wglGetProcAddress("glBindVertexBuffer");
    glVertexAttribFormat = (PFNGLVERTEXATTRIBFORMAT)wglGetProcAddress("glVertexAttribFormat");
    glVertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMAT)wglGetProcAddress("glVertexAttribIFormat");
    glVertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMAT)wglGetProcAddress("glVertexAttribLFormat");
    glVertexAttribBinding = (PFNGLVERTEXATTRIBBINDING)wglGetProcAddress("glVertexAttribBinding");
    glVertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISOR)wglGetProcAddress("glVertexBindingDivisor");
    // GL_ARB_direct_state_access
    glCreateVertexArrays = (PFNGLCREATEVERTEXARRAYS)wglGetProcAddress("glCreateVertexArrays");
    glDisableVertexArrayAttrib = (PFNGLDISABLEVERTEXARRAYATTRIB)wglGetProcAddress("glDisableVertexArrayAttrib");
    glEnableVertexArrayAttrib = (PFNGLENABLEVERTEXARRAYATTRIB)wglGetProcAddress("glEnableVertexArrayAttrib");
    glVertexArrayElementBuffer = (PFNGLVERTEXARRAYELEMENTBUFFER)wglGetProcAddress("glVertexArrayElementBuffer");
    glVertexArrayVertexBuffer = (PFNGLVERTEXARRAYVERTEXBUFFER)wglGetProcAddress("glVertexArrayVertexBuffer");
    glVertexArrayVertexBuffers = (PFNGLVERTEXARRAYVERTEXBUFFERS)wglGetProcAddress("glVertexArrayVertexBuffers");
    glVertexArrayAttribFormat = (PFNGLVERTEXARRAYATTRIBFORMAT)wglGetProcAddress("glVertexArrayAttribFormat");
    glVertexArrayAttribIFormat = (PFNGLVERTEXARRAYATTRIBIFORMAT)wglGetProcAddress("glVertexArrayAttribIFormat");
    glVertexArrayAttribLFormat = (PFNGLVERTEXARRAYATTRIBLFORMAT)wglGetProcAddress("glVertexArrayAttribLFormat");
    glVertexArrayAttribBinding = (PFNGLVERTEXARRAYATTRIBBINDING)wglGetProcAddress("glVertexArrayAttribBinding");
    glVertexArrayBindingDivisor = (PFNGLVERTEXARRAYBINDINGDIVISOR)wglGetProcAddress("glVertexArrayBindingDivisor");
}

//------------------------------------------------------------------------------
vertex_array::vertex_array()
    : _name(0)
{}

//------------------------------------------------------------------------------
vertex_array::vertex_array(vertex_array&& a) noexcept
    : _name(a._name)
{
    a._name = 0;
}

//------------------------------------------------------------------------------
vertex_array& vertex_array::operator=(vertex_array&& a) noexcept
{
    glDeleteVertexArrays(1, &_name);
    _name = a._name;
    a._name = 0;
    return *this;
}

//------------------------------------------------------------------------------
vertex_array::vertex_array(vertex_array_binding const* bindings, GLuint num_bindings)
    : _name(0)
{
    if (glCreateVertexArrays) {
        glCreateVertexArrays(1, &_name);

        // Workaround for Intel driver bug. Integer attributes are setup incorrectly
        // when using direct state access methods unless the vertex array is bound.
        glBindVertexArray(_name);

    } else if (glGenVertexArrays) {
        glGenVertexArrays(1, &_name);
        glBindVertexArray(_name);
    }

    for (GLuint attrib_index = 0, binding_index = 0; binding_index < num_bindings; ++binding_index) {
        vertex_array_binding const& binding = bindings[binding_index];

        for (GLuint ii = 0; ii < binding.num_attribs(); ++ii, ++attrib_index) {
            vertex_array_attrib const& attrib = binding.attrib(ii);
            switch (attrib.internalformat) {
                case vertex_attrib_type::float_:
                    if (glVertexArrayAttribFormat) {
                        glVertexArrayAttribFormat(_name, attrib_index, attrib.size, attrib.type, GL_FALSE, attrib.offset);
                    } else if (glVertexAttribFormat) {
                        glVertexAttribFormat(attrib_index, attrib.size, attrib.type, GL_FALSE, attrib.offset);
                    }
                    break;
                case vertex_attrib_type::normalized:
                    if (glVertexArrayAttribFormat) {
                        glVertexArrayAttribFormat(_name, attrib_index, attrib.size, attrib.type, GL_TRUE, attrib.offset);
                    } else if (glVertexAttribFormat) {
                        glVertexAttribFormat(attrib_index, attrib.size, attrib.type, GL_TRUE, attrib.offset);
                    }
                    break;
                case vertex_attrib_type::integer:
                    if (glVertexArrayAttribIFormat) {
                        glVertexArrayAttribIFormat(_name, attrib_index, attrib.size, attrib.type, attrib.offset);
                    } else if (glVertexAttribIFormat) {
                        glVertexAttribIFormat(attrib_index, attrib.size, attrib.type, attrib.offset);
                    }
                    break;
                case vertex_attrib_type::double_:
                    if (glVertexArrayAttribLFormat) {
                        glVertexArrayAttribLFormat(_name, attrib_index, attrib.size, attrib.type, attrib.offset);
                    } else if (glVertexAttribLFormat) {
                        glVertexAttribLFormat(attrib_index, attrib.size, attrib.type, attrib.offset);
                    }
                    break;
            }

            if (glEnableVertexArrayAttrib) {
                glEnableVertexArrayAttrib(_name, attrib_index);
            } else if (glEnableVertexAttribArray) {
                glEnableVertexAttribArray(attrib_index);
            }

            if (glVertexArrayAttribBinding) {
                glVertexArrayAttribBinding(_name, attrib_index, binding_index);
            } else if (glVertexAttribBinding) {
                glVertexAttribBinding(attrib_index, binding_index);
            }
        }

        if (glVertexArrayBindingDivisor) {
            glVertexArrayBindingDivisor(_name, binding_index, binding.divisor());
        } else if (glVertexBindingDivisor) {
            glVertexBindingDivisor(binding_index, binding.divisor());
        }
    }

    // Workaround for Intel driver bug, see comment above.
    glBindVertexArray(0);
}

//------------------------------------------------------------------------------
vertex_array::~vertex_array()
{
    glDeleteVertexArrays(1, &_name);
}

//------------------------------------------------------------------------------
void vertex_array::bind() const
{
    glBindVertexArray(_name);
}

//------------------------------------------------------------------------------
void vertex_array::bind_index_buffer(buffer const& b)
{
    if (glVertexArrayElementBuffer) {
        glVertexArrayElementBuffer(_name, b.name());
    } else {
        glBindVertexArray(_name);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b.name());
    }
}

//------------------------------------------------------------------------------
void vertex_array::bind_vertex_buffer(buffer const& b, GLuint bindindex, GLintptr offset, GLsizei stride)
{
    if (glVertexArrayVertexBuffer) {
        glVertexArrayVertexBuffer(_name, bindindex, b.name(), offset, stride);
    } else {
        glBindVertexArray(_name);
        glBindVertexBuffer(bindindex, b.name(), offset, stride);
    }
}

} // namespace gl
} // namespace render
