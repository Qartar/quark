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
    static void init();

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

protected:
    typedef void (APIENTRY* PFNGLBINDRENDERBUFFER)(GLenum target, GLuint renderbuffer);
    typedef void (APIENTRY* PFNGLDELETERENDERBUFFERS)(GLsizei n, GLuint const* renderbuffers);
    typedef void (APIENTRY* PFNGLGENRENDERBUFFERS)(GLsizei n, GLuint* renderbuffers);
    typedef void (APIENTRY* PFNGLRENDERBUFFERSTORAGE)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    typedef void (APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
    typedef void (APIENTRY* PFNGLBINDFRAMEBUFFER)(GLenum target, GLuint framebuffer);
    typedef void (APIENTRY* PFNGLDELETEFRAMEBUFFERS)(GLsizei n, GLuint const* framebuffers);
    typedef void (APIENTRY* PFNGLGENFRAMEBUFFERS)(GLsizei n, GLuint* framebuffers);
    typedef void (APIENTRY* PFNGLFRAMEBUFFERRENDERBUFFER)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    typedef void (APIENTRY* PFNGLFRAMEBUFFERTEXTURE2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    typedef void (APIENTRY* PFNGLBLITFRAMEBUFFER)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

    typedef void (APIENTRY* PFNGLDRAWBUFFERS)(GLsizei n, GLenum const* bufs);

    static PFNGLBINDRENDERBUFFER glBindRenderbuffer;
    static PFNGLDELETERENDERBUFFERS glDeleteRenderbuffers;
    static PFNGLGENRENDERBUFFERS glGenRenderbuffers;
    static PFNGLRENDERBUFFERSTORAGE glRenderbufferStorage;
    static PFNGLRENDERBUFFERSTORAGEMULTISAMPLE glRenderbufferStorageMultisample;
    static PFNGLBINDFRAMEBUFFER glBindFramebuffer;
    static PFNGLDELETEFRAMEBUFFERS glDeleteFramebuffers;
    static PFNGLGENFRAMEBUFFERS glGenFramebuffers;
    static PFNGLFRAMEBUFFERRENDERBUFFER glFramebufferRenderbuffer;
    static PFNGLFRAMEBUFFERTEXTURE2D glFramebufferTexture2D;
    static PFNGLBLITFRAMEBUFFER glBlitFramebuffer;

    static PFNGLDRAWBUFFERS glDrawBuffers;
};

} // namespace gl
} // namespace render
