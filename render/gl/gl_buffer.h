// gl_buffer.h
//

#pragma once

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

//------------------------------------------------------------------------------
enum class buffer_type
{
    vertex,
    index,
    uniform,
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
    buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr size);
    buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr size, void const* data);
    ~buffer();

    GLuint name() const { return _name; }
    GLsizeiptr size() const { return _size; }

    void bind() const;
    void bind_base(GLuint index) const;
    void bind_range(GLuint index, GLintptr offset, GLsizeiptr size) const;

protected:
    buffer_type _type;
    buffer_usage _usage;
    buffer_access _access;

    GLuint _name;
    GLsizeiptr _size;
    void* _data;

protected:
    // OpenGL 1.5
    typedef void (APIENTRY* PFNGLBINDBUFFER)(GLenum target, GLuint buffer);
    typedef void (APIENTRY* PFNGLDELETEBUFFERS)(GLsizei n, GLuint const* buffers);
    typedef void (APIENTRY* PFNGLGENBUFFERS)(GLsizei n, GLuint* buffers);
    typedef void (APIENTRY* PFNGLBUFFERDATA)(GLenum target, GLsizeiptr size, void const* data, GLenum usage);
    typedef void (APIENTRY* PFNGLBUFFERSUBDATA)(GLenum target, GLintptr offset, GLsizeiptr size, void const* data);
    typedef void* (APIENTRY* PFNGLMAPBUFFER)(GLenum target, GLenum access);
    typedef void (APIENTRY* PFNGLUNMAPBUFFER)(GLenum target);

    static PFNGLBINDBUFFER glBindBuffer;
    static PFNGLDELETEBUFFERS glDeleteBuffers;
    static PFNGLGENBUFFERS glGenBuffers;
    static PFNGLBUFFERDATA glBufferData;
    static PFNGLBUFFERSUBDATA glBufferSubData;
    static PFNGLMAPBUFFER glMapBuffer;
    static PFNGLUNMAPBUFFER glUnmapBuffer;

    // OpenGL 3.0
    typedef void (APIENTRY* PFNGLBINDBUFFERRANGE)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
    typedef void (APIENTRY* PFNGLBINDBUFFERBASE)(GLenum target, GLuint index, GLuint buffer);

    static PFNGLBINDBUFFERRANGE glBindBufferRange;
    static PFNGLBINDBUFFERBASE glBindBufferBase;
};

//------------------------------------------------------------------------------
template<typename T> class typed_buffer : public buffer
{
protected:
    typed_buffer()
        : buffer() {}
    typed_buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr num_elements)
        : buffer(type, usage, access, num_elements * sizeof(T)) {}
    typed_buffer(buffer_type type, buffer_usage usage, buffer_access access, GLsizeiptr num_elements, T const* elements)
        : buffer(type, usage, access, num_elements * sizeof(T), elements) {}

public:
    using element_type = T;

    GLsizeiptr num_elements() const { return _size / sizeof(element_type); }
};

//------------------------------------------------------------------------------
template<typename T> class vertex_buffer : public typed_buffer<T>
{
public:
    vertex_buffer()
        : typed_buffer<T>() {}
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
        : typed_buffer<T>() {}
    index_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_indices)
        : typed_buffer<T>(buffer_type::index, usage, access, num_indices) {}
    index_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_indices, T const* indices)
        : typed_buffer<T>(buffer_type::index, usage, access, num_indices, indices) {}
};

//------------------------------------------------------------------------------
template<typename T> class uniform_buffer : public typed_buffer<T>
{
public:
    uniform_buffer()
        : typed_buffer<T>() {}
    uniform_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_uniforms)
        : typed_buffer<T>(buffer_type::index, usage, access, num_uniforms) {}
    uniform_buffer(buffer_usage usage, buffer_access access, GLsizeiptr num_uniforms, T const* uniforms)
        : typed_buffer<T>(buffer_type::index, usage, access, num_uniforms, uniforms) {}
};

} // namespace gl
} // namespace render
