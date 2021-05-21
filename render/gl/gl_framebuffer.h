// gl_framebuffer.h
//

#pragma once

#include "gl/gl_types.h"
#include "gl/gl_texture.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

class framebuffer;

//------------------------------------------------------------------------------
enum class attachment_type {
    color,
    depth,
    stencil,
    depth_stencil,
};

//------------------------------------------------------------------------------
class attachment
{
public:
    attachment(attachment_type type, GLenum internalformat)
        : _type(type)
        , _format(internalformat)
    {}

protected:
    friend framebuffer;
    attachment_type _type;
    GLenum _format;
};

//------------------------------------------------------------------------------
class framebuffer
{
public:
    //! Initialize function pointers
    static void init(get_proc_address_t get_proc_address);

    framebuffer();
    framebuffer(framebuffer&& f) noexcept;
    framebuffer& operator=(framebuffer&& f) noexcept;
    framebuffer(GLsizei width, GLsizei height, attachment const* attachments, std::size_t num_attachments);
    framebuffer(GLsizei samples, GLsizei width, GLsizei height, attachment const* attachments, std::size_t num_attachments);
    ~framebuffer();

    template<std::size_t num_attachments>
    framebuffer(GLsizei width, GLsizei height, attachment const (&attachments)[num_attachments])
        : framebuffer(width, height, attachments, num_attachments)
    {}

    template<std::size_t num_attachments>
    framebuffer(GLsizei samples, GLsizei width, GLsizei height, attachment const (&attachments)[num_attachments])
        : framebuffer(samples, width, height, attachments, num_attachments)
    {}

    GLsizei width() const { return _width; }
    GLsizei height() const { return _height; }

    void read() const;
    void draw() const;

    void read_buffer(GLenum buf) const;
    void draw_buffer(GLenum buf) const;
    void draw_buffers(GLsizei n, GLenum const* bufs) const;

    template<std::size_t n>
    void draw_buffers(GLenum const (&bufs)[n]) const { draw_buffers(n, bufs); }

    void blit(framebuffer const& src, GLbitfield mask, GLenum filter) const;
    void blit(framebuffer const& src, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const;

    texture const& color_attachment(std::size_t index = 0) const { return _color_attachments[index]; }
    texture const& depth_attachment() const { return _depth_stencil_attachment.name() ? _depth_stencil_attachment : _depth_attachment; }
    texture const& stencil_attachment() const { return _depth_stencil_attachment.name() ? _depth_stencil_attachment : _stencil_attachment; }

protected:
    GLuint _name;
    GLsizei _width;
    GLsizei _height;

    std::vector<texture> _color_attachments;
    texture _depth_attachment;
    texture _stencil_attachment;
    texture _depth_stencil_attachment;

protected:
    framebuffer(framebuffer const&) = delete;
    framebuffer& operator=(framebuffer const&) = delete;

    void attach(GLenum attachment, GLenum target, GLuint texture, GLuint level);

protected:
    // GL_ARB_framebuffer_object
    using PFNGLBINDFRAMEBUFFER = void (APIENTRY*)(GLenum target, GLuint framebuffer);
    using PFNGLDELETEFRAMEBUFFERS = void (APIENTRY*)(GLsizei n, GLuint const* framebuffers);
    using PFNGLGENFRAMEBUFFERS = void (APIENTRY*)(GLsizei n, GLuint* framebuffers);
    using PFNGLFRAMEBUFFERTEXTURE2D = void (APIENTRY*)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    using PFNGLBLITFRAMEBUFFER = void (APIENTRY*)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

    static PFNGLBINDFRAMEBUFFER glBindFramebuffer;
    static PFNGLDELETEFRAMEBUFFERS glDeleteFramebuffers;
    static PFNGLGENFRAMEBUFFERS glGenFramebuffers;
    static PFNGLFRAMEBUFFERTEXTURE2D glFramebufferTexture2D;
    static PFNGLBLITFRAMEBUFFER glBlitFramebuffer;

    // GL_ARB_direct_state_access
    using PFNGLCREATEFRAMEBUFFERS = void (APIENTRY*)(GLsizei n, GLuint* framebuffers);
    using PFNGLNAMEDFRAMEBUFFERTEXTURE = void (APIENTRY*)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
    using PFNGLNAMEDFRAMEBUFFERDRAWBUFFER = void (APIENTRY*)(GLuint framebuffer, GLenum mode);
    using PFNGLNAMEDFRAMEBUFFERDRAWBUFFERS = void (APIENTRY*)(GLuint framebuffer, GLsizei n, GLenum const* bufs);
    using PFNGLNAMEDFRAMEBUFFERREADBUFFER = void (APIENTRY*)(GLuint framebuffer, GLenum mode);
    using PFNGLBLITNAMEDFRAMEBUFFER = void (APIENTRY*)(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

    static PFNGLCREATEFRAMEBUFFERS glCreateFramebuffers;
    static PFNGLNAMEDFRAMEBUFFERTEXTURE glNamedFramebufferTexture;
    static PFNGLNAMEDFRAMEBUFFERDRAWBUFFER glNamedFramebufferDrawBuffer;
    static PFNGLNAMEDFRAMEBUFFERDRAWBUFFERS glNamedFramebufferDrawBuffers;
    static PFNGLNAMEDFRAMEBUFFERREADBUFFER glNamedFramebufferReadBuffer;
    static PFNGLBLITNAMEDFRAMEBUFFER glBlitNamedFramebuffer;

    using PFNGLDRAWBUFFERS = void (APIENTRY*)(GLsizei n, GLenum const* bufs);

    static PFNGLDRAWBUFFERS glDrawBuffers;
};

} // namespace gl
} // namespace render
