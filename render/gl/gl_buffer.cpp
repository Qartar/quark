// gl_buffer.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "gl/gl_buffer.h"
#include "gl/gl_include.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

namespace {

//------------------------------------------------------------------------------
constexpr GLenum gl_buffer_target(buffer_type type)
{
    switch (type) {
        case buffer_type::vertex:
            return GL_ARRAY_BUFFER;
        case buffer_type::index:
            return GL_ELEMENT_ARRAY_BUFFER;
        case buffer_type::uniform:
            return GL_UNIFORM_BUFFER;
        case buffer_type::shader_storage:
            return GL_SHADER_STORAGE_BUFFER;
    }
     // rely on compiler warnings to detect missing case labels
    __assume(false);
}

//------------------------------------------------------------------------------
constexpr GLenum gl_buffer_usage(buffer_usage usage, buffer_access access)
{
    constexpr GLenum usage_table[4][3] = {
        { GL_STATIC_COPY, GL_STATIC_DRAW, GL_STATIC_READ },
        { GL_STREAM_COPY, GL_STREAM_DRAW, GL_STREAM_READ },
        { GL_DYNAMIC_COPY, GL_DYNAMIC_DRAW, GL_DYNAMIC_READ },
        { GL_STATIC_COPY, GL_STATIC_DRAW, GL_STATIC_READ },
    };
    return usage_table[static_cast<int>(usage)][static_cast<int>(access)];
}

//------------------------------------------------------------------------------
constexpr GLenum gl_buffer_access(buffer_access access)
{
    switch (access) {
        case buffer_access::copy:
            return 0;
        case buffer_access::draw:
            return GL_WRITE_ONLY;
        case buffer_access::read:
            return GL_READ_ONLY;
    }
     // rely on compiler warnings to detect missing case labels
    __assume(false);
}

//------------------------------------------------------------------------------
constexpr GLenum gl_buffer_access_bits(buffer_access access)
{
    switch (access) {
        case buffer_access::copy:
            return 0;
        case buffer_access::draw:
            return GL_MAP_WRITE_BIT;
        case buffer_access::read:
            return GL_MAP_READ_BIT;
    }
     // rely on compiler warnings to detect missing case labels
    __assume(false);
}

} // anonymous namespace

// OpenGL 1.5
buffer::PFNGLBINDBUFFER buffer::glBindBuffer = nullptr;
buffer::PFNGLDELETEBUFFERS buffer::glDeleteBuffers = nullptr;
buffer::PFNGLGENBUFFERS buffer::glGenBuffers = nullptr;
buffer::PFNGLBUFFERDATA buffer::glBufferData = nullptr;
buffer::PFNGLBUFFERSUBDATA buffer::glBufferSubData = nullptr;
buffer::PFNGLMAPBUFFER buffer::glMapBuffer = nullptr;
buffer::PFNGLUNMAPBUFFER buffer::glUnmapBuffer = nullptr;
// OpenGL 3.0
buffer::PFNGLBINDBUFFERRANGE buffer::glBindBufferRange = nullptr;
buffer::PFNGLBINDBUFFERBASE buffer::glBindBufferBase = nullptr;

//------------------------------------------------------------------------------
void buffer::init()
{
    // OpenGL 1.5
    glBindBuffer = (PFNGLBINDBUFFER)wglGetProcAddress("glBindBuffer");
    glDeleteBuffers = (PFNGLDELETEBUFFERS)wglGetProcAddress("glDeleteBuffers");
    glGenBuffers = (PFNGLGENBUFFERS)wglGetProcAddress("glGenBuffers");
    glBufferData = (PFNGLBUFFERDATA)wglGetProcAddress("glBufferData");
    glBufferSubData = (PFNGLBUFFERSUBDATA)wglGetProcAddress("glBufferSubData");
    glMapBuffer = (PFNGLMAPBUFFER)wglGetProcAddress("glMapBuffer");
    glUnmapBuffer = (PFNGLUNMAPBUFFER)wglGetProcAddress("glUnmapBuffer");
    // OpenGL 3.0
    glBindBufferRange = (PFNGLBINDBUFFERRANGE)wglGetProcAddress("glBindBufferRange");
    glBindBufferBase = (PFNGLBINDBUFFERBASE)wglGetProcAddress("glBindBufferBase");
}

//------------------------------------------------------------------------------
buffer::buffer()
    : _type{}
    , _usage{}
    , _access{}
    , _name(0)
    , _size{}
    , _data{}
{}

//------------------------------------------------------------------------------
buffer::buffer(buffer_type type)
    : _type(type)
    , _usage{}
    , _access{}
    , _name(0)
    , _size{}
    , _data{}
{}

//------------------------------------------------------------------------------
buffer::buffer(buffer&& b) noexcept
    : _type(b._type)
    , _usage(b._usage)
    , _access(b._access)
    , _name(b._name)
    , _size(b._size)
    , _data(b._data)
{
    b._name = 0;
}

//------------------------------------------------------------------------------
buffer& buffer::operator=(buffer&& b) noexcept
{
    glDeleteBuffers(1, &_name);
    _type = b._type;
    _usage = b._usage;
    _access = b._access;
    _name = b._name;
    _size = b._size;
    _data = b._data;
    b._data = nullptr;
    b._name = 0;
    return *this;
}

//------------------------------------------------------------------------------
buffer::buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr size)
    : buffer(type, usage, access, size, nullptr)
{
}

//------------------------------------------------------------------------------
buffer::buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr size, void const* data)
    : _type(type)
    , _usage(usage)
    , _access(access)
    , _name(0)
    , _size(size)
    , _data(nullptr)
{
    glGenBuffers(1, &_name);
    glBindBuffer(gl_buffer_target(_type), _name);
    glBufferData(gl_buffer_target(_type), _size, data, gl_buffer_usage(_usage, _access));
}

//------------------------------------------------------------------------------
buffer::~buffer()
{
    glDeleteBuffers(1, &_name);
}

//------------------------------------------------------------------------------
void buffer::bind() const
{
    glBindBuffer(gl_buffer_target(_type), _name);
}

//------------------------------------------------------------------------------
void buffer::bind_base(GLuint index) const
{
    glBindBufferBase(gl_buffer_target(_type), index, _name);
}

//------------------------------------------------------------------------------
void buffer::bind_range(GLuint index, GLintptr offset, GLsizeiptr size) const
{
    glBindBufferRange(gl_buffer_target(_type), index, _name, offset, size);
}

//------------------------------------------------------------------------------
void buffer::upload(GLintptr offset, GLsizeiptr size, void const* data) const
{
    glBindBuffer(gl_buffer_target(_type), _name);
    glBufferSubData(gl_buffer_target(_type), offset, size, data);
}

} // namespace gl
} // namespace render
