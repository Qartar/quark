// gl_buffer.h
//

#pragma once

#include "gl/gl_types.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

//------------------------------------------------------------------------------
enum class buffer_type
{
    vertex,
    index,
    uniform,
    shader_storage,
};

//------------------------------------------------------------------------------
enum class buffer_usage
{
    static_,
    stream,
    dynamic,
    persistent,
};

//------------------------------------------------------------------------------
enum class buffer_access
{
    copy,
    draw,
    read,
};

//------------------------------------------------------------------------------
class buffer
{
public:
    static void init();

    buffer();
    buffer(buffer&& b) noexcept;
    buffer& operator=(buffer&& b) noexcept;
    buffer(buffer_type type);
    buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr size);
    buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr size, void const* data);
    ~buffer();

    GLuint name() const { return _name; }
    GLsizeiptr size() const { return _size; }

    void bind() const;
    void bind_base(GLuint index) const;
    void bind_range(GLuint index, GLintptr offset, GLsizeiptr size) const;

    void upload(GLintptr offset, GLsizeiptr size, void const* data) const;

protected:
    buffer_type _type;
    buffer_usage _usage;
    buffer_access _access;

    GLuint _name;
    GLsizeiptr _size;
    void* _data;

protected:
    // OpenGL 1.5
    using PFNGLBINDBUFFER = void (APIENTRY*)(GLenum target, GLuint buffer);
    using PFNGLDELETEBUFFERS = void (APIENTRY*)(GLsizei n, GLuint const* buffers);
    using PFNGLGENBUFFERS = void (APIENTRY*)(GLsizei n, GLuint* buffers);
    using PFNGLBUFFERDATA = void (APIENTRY*)(GLenum target, GLsizeiptr size, void const* data, GLenum usage);
    using PFNGLBUFFERSUBDATA = void (APIENTRY*)(GLenum target, GLintptr offset, GLsizeiptr size, void const* data);
    using PFNGLMAPBUFFER = void* (APIENTRY*)(GLenum target, GLenum access);
    using PFNGLUNMAPBUFFER = GLboolean (APIENTRY*)(GLenum target);

    static PFNGLBINDBUFFER glBindBuffer;
    static PFNGLDELETEBUFFERS glDeleteBuffers;
    static PFNGLGENBUFFERS glGenBuffers;
    static PFNGLBUFFERDATA glBufferData;
    static PFNGLBUFFERSUBDATA glBufferSubData;
    static PFNGLMAPBUFFER glMapBuffer;
    static PFNGLUNMAPBUFFER glUnmapBuffer;

    // OpenGL 3.0
    using PFNGLBINDBUFFERRANGE = void (APIENTRY*)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
    using PFNGLBINDBUFFERBASE = void (APIENTRY*)(GLenum target, GLuint index, GLuint buffer);

    static PFNGLBINDBUFFERRANGE glBindBufferRange;
    static PFNGLBINDBUFFERBASE glBindBufferBase;

    // GL_ARB_direct_state_access
    using PFNGLCREATEBUFFERS = void (APIENTRY*)(GLsizei n, GLuint* buffers);
    using PFNGLNAMEDBUFFERDATA = void (APIENTRY*)(GLuint buffer, GLsizeiptr size, void const* data, GLenum usage);
    using PFNGLNAMEDBUFFERSUBDATA = void (APIENTRY*)(GLuint buffer, GLintptr offset, GLsizeiptr size, void const* data);
    using PFNGLMAPNAMEDBUFFER = void* (APIENTRY*)(GLuint buffer, GLenum access);
    using PFNGLUNMAPNAMEDBUFFER = GLboolean (APIENTRY*)(GLuint buffer);

    static PFNGLCREATEBUFFERS glCreateBuffers;
    static PFNGLNAMEDBUFFERDATA glNamedBufferData;
    static PFNGLNAMEDBUFFERSUBDATA glNamedBufferSubData;
    static PFNGLMAPNAMEDBUFFER glMapNamedBuffer;
    static PFNGLUNMAPNAMEDBUFFER glUnmapNamedBuffer;
};

//------------------------------------------------------------------------------
template<typename T> class typed_buffer : public buffer
{
protected:
    typed_buffer()
        : buffer() {}
    typed_buffer(buffer_type type)
        : buffer(type) {}
    typed_buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr num_elements)
        : buffer(type, usage, access, num_elements * sizeof(T)) {}
    typed_buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr num_elements, T const* elements)
        : buffer(type, usage, access, num_elements * sizeof(T), elements) {}

    using buffer::upload; //! Make typeless signature protected to avoid unintended type coercion

public:
    using element_type = T;

    GLsizeiptr num_elements() const { return _size / sizeof(element_type); }

    void upload(GLintptr first_element, GLsizeiptr num_elements, T const* elements) const {
        return buffer::upload(first_element * sizeof(T), num_elements * sizeof(T), elements);
    }
};

//------------------------------------------------------------------------------
template<typename T> class vertex_buffer : public typed_buffer<T>
{
public:
    vertex_buffer()
        : typed_buffer<T>(buffer_type::vertex) {}
    vertex_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_vertices)
        : typed_buffer<T>(buffer_type::vertex, usage, access, num_vertices) {}
    vertex_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_vertices, T const* vertices)
        : typed_buffer<T>(buffer_type::vertex, usage, access, num_vertices, vertices) {}
    template<std::size_t num_vertices>
    vertex_buffer(buffer_usage usage, buffer_access access, T const (&vertices)[num_vertices])
        : typed_buffer<T>(buffer_type::vertex, usage, access, num_vertices, vertices) {}
};

//------------------------------------------------------------------------------
template<typename T> class index_buffer : public typed_buffer<T>
{
public:
    index_buffer()
        : typed_buffer<T>(buffer_type::index) {}
    index_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_indices)
        : typed_buffer<T>(buffer_type::index, usage, access, num_indices) {}
    index_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_indices, T const* indices)
        : typed_buffer<T>(buffer_type::index, usage, access, num_indices, indices) {}
    template<std::size_t num_indices>
    index_buffer(buffer_usage usage, buffer_access access, T const (&indices)[num_indices])
        : typed_buffer<T>(buffer_type::index, usage, access, num_indices, indices) {}
};

//------------------------------------------------------------------------------
template<typename T> class uniform_buffer : public typed_buffer<T>
{
public:
    uniform_buffer()
        : typed_buffer<T>(buffer_type::uniform) {}
    uniform_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_uniforms)
        : typed_buffer<T>(buffer_type::uniform, usage, access, num_uniforms) {}
    uniform_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_uniforms, T const* uniforms)
        : typed_buffer<T>(buffer_type::uniform, usage, access, num_uniforms, uniforms) {}
};

//------------------------------------------------------------------------------
template<typename T> class shader_storage_buffer : public typed_buffer<T>
{
public:
    shader_storage_buffer()
        : typed_buffer<T>(buffer_type::shader_storage) {}
    shader_storage_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_uniforms)
        : typed_buffer<T>(buffer_type::shader_storage, usage, access, num_uniforms) {}
    shader_storage_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_uniforms, T const* uniforms)
        : typed_buffer<T>(buffer_type::shader_storage, usage, access, num_uniforms, uniforms) {}
};

} // namespace gl
} // namespace render
