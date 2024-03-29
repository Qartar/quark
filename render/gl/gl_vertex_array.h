// gl_vertex_array.h
//

#pragma once

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

class buffer;
template<typename> class index_buffer;
template<typename> class vertex_buffer;

//------------------------------------------------------------------------------
enum class vertex_attrib_type
{
    float_,
    normalized,
    integer,
    double_,
};

//------------------------------------------------------------------------------
struct vertex_array_attrib
{
    GLint size;
    GLenum type;
    vertex_attrib_type internalformat;
    GLuint offset;
};

//------------------------------------------------------------------------------
class vertex_array_binding
{
public:
    static constexpr std::size_t max_attribs = 16;

    template<std::size_t num_attribs>
    vertex_array_binding(GLuint divisor, vertex_array_attrib const (&attribs)[num_attribs])
        : _divisor(divisor)
        , _num_attribs(num_attribs)
    {
        static_assert(num_attribs <= max_attribs);
        memcpy(_attribs, attribs, sizeof(attribs));
    }

    template<std::size_t num_attribs>
    vertex_array_binding(vertex_array_attrib const (&attribs)[num_attribs])
        : vertex_array_binding(0, attribs) {}

    GLuint divisor() const { return _divisor; }
    vertex_array_attrib const& attrib(std::size_t index) const { return _attribs[index]; }
    std::size_t num_attribs() const { return _num_attribs; }

protected:
    GLuint _divisor;
    vertex_array_attrib _attribs[max_attribs];
    std::size_t _num_attribs;
};

//------------------------------------------------------------------------------
class vertex_array
{
public:
    static void init();

    vertex_array();
    vertex_array(vertex_array&& a) noexcept;
    vertex_array& operator=(vertex_array&& a) noexcept;
    ~vertex_array();

    template<GLuint num_bindings>
    vertex_array(vertex_array_binding const (&bindings)[num_bindings])
        : vertex_array(bindings, num_bindings)
    {}

    template<GLuint num_attribs>
    vertex_array(vertex_array_attrib const (&attribs)[num_attribs])
        : vertex_array({vertex_array_binding{attribs}})
    {}

    void bind() const;

    template<typename T>
    void bind_buffer(index_buffer<T> const& buffer) {
        return bind_index_buffer(buffer);
    }

    template<typename T>
    void bind_buffer(vertex_buffer<T> const& buffer, GLuint bindindex, GLintptr offset = 0, GLsizei stride = 1) {
        return bind_vertex_buffer(buffer, bindindex, offset * sizeof(T), stride * sizeof(T));
    }

protected:
    GLuint _name;

protected:
    vertex_array(vertex_array_binding const* bindings, GLuint num_bindings);

    void bind_index_buffer(buffer const& b);
    void bind_vertex_buffer(buffer const& b, GLuint bindindex, GLintptr offset, GLsizei stride);

protected:
    // OpenGL 3.0
    using PFNGLBINDVERTEXARRAY = void (APIENTRY*)(GLuint array);
    using PFNGLDELETEVERTEXARRAYS = void (APIENTRY*)(GLsizei n, GLuint const* arrays);
    using PFNGLGENVERTEXARRAYS = void (APIENTRY*)(GLsizei n, GLuint* arrays);

    static PFNGLBINDVERTEXARRAY glBindVertexArray;
    static PFNGLDELETEVERTEXARRAYS glDeleteVertexArrays;
    static PFNGLGENVERTEXARRAYS glGenVertexArrays;

    // OpenGL 4.3
    // ...

    // GL_ARB_direct_state_access
    using PFNGLCREATEVERTEXARRAYS = void (APIENTRY*)(GLsizei n, GLuint* arrays);
    using PFNGLDISABLEVERTEXARRAYATTRIB = void (APIENTRY*)(GLuint vaobj, GLuint index);
    using PFNGLENABLEVERTEXARRAYATTRIB = void (APIENTRY*)(GLuint vaobj, GLuint index);
    using PFNGLVERTEXARRAYELEMENTBUFFER = void (APIENTRY*)(GLuint vaobj, GLuint buffer);
    using PFNGLVERTEXARRAYVERTEXBUFFER = void (APIENTRY*)(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
    using PFNGLVERTEXARRAYVERTEXBUFFERS = void (APIENTRY*)(GLuint vaobj, GLuint first, GLsizei count, GLuint const* buffers, GLintptr const* offsets, GLsizei const* strides);
    using PFNGLVERTEXARRAYATTRIBFORMAT = void (APIENTRY*)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
    using PFNGLVERTEXARRAYATTRIBIFORMAT = void (APIENTRY*)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
    using PFNGLVERTEXARRAYATTRIBLFORMAT = void (APIENTRY*)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
    using PFNGLVERTEXARRAYATTRIBBINDING = void (APIENTRY*)(GLuint vaobj, GLuint attribindex, GLuint bindingindex);
    using PFNGLVERTEXARRAYBINDINGDIVISOR = void (APIENTRY*)(GLuint vaobj, GLuint bindingindex, GLuint divisor);

    static PFNGLCREATEVERTEXARRAYS glCreateVertexArrays;
    static PFNGLDISABLEVERTEXARRAYATTRIB glDisableVertexArrayAttrib;
    static PFNGLENABLEVERTEXARRAYATTRIB glEnableVertexArrayAttrib;
    static PFNGLVERTEXARRAYELEMENTBUFFER glVertexArrayElementBuffer;
    static PFNGLVERTEXARRAYVERTEXBUFFER glVertexArrayVertexBuffer;
    static PFNGLVERTEXARRAYVERTEXBUFFERS glVertexArrayVertexBuffers;
    static PFNGLVERTEXARRAYATTRIBFORMAT glVertexArrayAttribFormat;
    static PFNGLVERTEXARRAYATTRIBIFORMAT glVertexArrayAttribIFormat;
    static PFNGLVERTEXARRAYATTRIBLFORMAT glVertexArrayAttribLFormat;
    static PFNGLVERTEXARRAYATTRIBBINDING glVertexArrayAttribBinding;
    static PFNGLVERTEXARRAYBINDINGDIVISOR glVertexArrayBindingDivisor;
};

} // namespace gl
} // namespace render
