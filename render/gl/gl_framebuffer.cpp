// gl_framebuffer.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "gl/gl_include.h"
#include "gl/gl_framebuffer.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

// GL_ARB_framebuffer_object
framebuffer::PFNGLBINDFRAMEBUFFER framebuffer::glBindFramebuffer = nullptr;
framebuffer::PFNGLDELETEFRAMEBUFFERS framebuffer::glDeleteFramebuffers = nullptr;
framebuffer::PFNGLGENFRAMEBUFFERS framebuffer::glGenFramebuffers = nullptr;
framebuffer::PFNGLFRAMEBUFFERTEXTURE2D framebuffer::glFramebufferTexture2D = nullptr;
framebuffer::PFNGLBLITFRAMEBUFFER framebuffer::glBlitFramebuffer = nullptr;

// GL_ARB_direct_state_access
framebuffer::PFNGLCREATEFRAMEBUFFERS framebuffer::glCreateFramebuffers = nullptr;
framebuffer::PFNGLNAMEDFRAMEBUFFERTEXTURE framebuffer::glNamedFramebufferTexture = nullptr;
framebuffer::PFNGLNAMEDFRAMEBUFFERDRAWBUFFER framebuffer::glNamedFramebufferDrawBuffer = nullptr;
framebuffer::PFNGLNAMEDFRAMEBUFFERDRAWBUFFERS framebuffer::glNamedFramebufferDrawBuffers = nullptr;
framebuffer::PFNGLNAMEDFRAMEBUFFERREADBUFFER framebuffer::glNamedFramebufferReadBuffer = nullptr;
framebuffer::PFNGLBLITNAMEDFRAMEBUFFER framebuffer::glBlitNamedFramebuffer = nullptr;

framebuffer::PFNGLDRAWBUFFERS framebuffer::glDrawBuffers = nullptr;

//------------------------------------------------------------------------------
void framebuffer::init()
{
    // GL_ARB_framebuffer_object
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFER )wglGetProcAddress("glBindFramebuffer");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERS )wglGetProcAddress("glDeleteFramebuffers");
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERS )wglGetProcAddress("glGenFramebuffers");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2D )wglGetProcAddress("glFramebufferTexture2D");
    glBlitFramebuffer = (PFNGLBLITFRAMEBUFFER )wglGetProcAddress("glBlitFramebuffer");

    // GL_ARB_direct_state_access
    glCreateFramebuffers = (PFNGLCREATEFRAMEBUFFERS )wglGetProcAddress("glCreateFramebuffers");
    glNamedFramebufferTexture = (PFNGLNAMEDFRAMEBUFFERTEXTURE )wglGetProcAddress("glNamedFramebufferTexture");
    glNamedFramebufferDrawBuffer = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFER )wglGetProcAddress("glNamedFramebufferDrawBuffer");
    glNamedFramebufferDrawBuffers = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFERS )wglGetProcAddress("glNamedFramebufferDrawBuffers");
    glNamedFramebufferReadBuffer = (PFNGLNAMEDFRAMEBUFFERREADBUFFER )wglGetProcAddress("glNamedFramebufferReadBuffer");
    glBlitNamedFramebuffer = (PFNGLBLITNAMEDFRAMEBUFFER )wglGetProcAddress("glBlitNamedFramebuffer");

    glDrawBuffers = (PFNGLDRAWBUFFERS )wglGetProcAddress("glDrawBuffers");
}

//------------------------------------------------------------------------------
framebuffer::framebuffer()
    : _name(0)
    , _width(0)
    , _height(0)
{}

//------------------------------------------------------------------------------
framebuffer::framebuffer(framebuffer&& f) noexcept
    : _name(f._name)
    , _width(f._width)
    , _height(f._height)
    , _color_attachments(std::move(f._color_attachments))
    , _depth_attachment(std::move(f._depth_attachment))
    , _stencil_attachment(std::move(f._stencil_attachment))
    , _depth_stencil_attachment(std::move(f._depth_stencil_attachment))
{
    f._name = 0;
}

//------------------------------------------------------------------------------
framebuffer& framebuffer::operator=(framebuffer&& f) noexcept
{
    glDeleteFramebuffers(1, &_name);
    _name = f._name;
    _width = f._width;
    _height = f._height;
    _color_attachments = std::move(f._color_attachments);
    _depth_attachment = std::move(f._depth_attachment);
    _stencil_attachment = std::move(f._stencil_attachment);
    _depth_stencil_attachment = std::move(f._depth_stencil_attachment);
    f._name = 0;
    return *this;
}

//------------------------------------------------------------------------------
framebuffer::framebuffer(GLsizei width, GLsizei height, attachment const* attachments, std::size_t num_attachments)
    : _name(0)
    , _width(width)
    , _height(height)
{
    if (glCreateFramebuffers) {
        glCreateFramebuffers(1, &_name);
    } else if (glGenFramebuffers) {
        glGenFramebuffers(1, &_name);
        glBindFramebuffer(GL_FRAMEBUFFER, _name);
    }

    std::size_t num_color_attachments = 0;
    std::size_t num_depth_attachments = 0;
    std::size_t num_stencil_attachments = 0;
    std::size_t num_depth_stencil_attachments = 0;

    for (std::size_t ii = 0; ii < num_attachments; ++ii) {
        switch (attachments[ii]._type) {
            case attachment_type::color:
                _color_attachments.push_back(texture2d(1, attachments[ii]._format, _width, _height));
                attach(
                    narrow_cast<GLenum>(GL_COLOR_ATTACHMENT0 + num_color_attachments),
                    GL_TEXTURE_2D,
                    _color_attachments[num_color_attachments].name(),
                    0);
                ++num_color_attachments;
                break;

            case attachment_type::depth:
                if (num_depth_attachments || num_depth_stencil_attachments) {
                    continue;
                }
                _depth_attachment = texture2d(1, attachments[ii]._format, _width, _height);
                attach(
                    GL_DEPTH_ATTACHMENT,
                    GL_TEXTURE_2D,
                    _depth_attachment.name(),
                    0);
                ++num_depth_attachments;
                break;

            case attachment_type::stencil:
                if (num_stencil_attachments || num_depth_stencil_attachments) {
                    continue;
                }
                _stencil_attachment = texture2d(1, attachments[ii]._format, _width, _height);
                attach(
                    GL_STENCIL_ATTACHMENT,
                    GL_TEXTURE_2D,
                    _stencil_attachment.name(),
                    0);
                ++num_stencil_attachments;
                break;

            case attachment_type::depth_stencil:
                if (num_depth_attachments || num_stencil_attachments) {
                    continue;
                }
                _depth_stencil_attachment = texture2d(1, attachments[ii]._format, _width, _height);
                attach(
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    GL_TEXTURE_2D,
                    _depth_stencil_attachment.name(),
                    0);
                ++num_depth_stencil_attachments;
                break;
        }
    }
}

//------------------------------------------------------------------------------
framebuffer::framebuffer(GLsizei samples, GLsizei width, GLsizei height, attachment const* attachments, std::size_t num_attachments)
    : _name(0)
    , _width(width)
    , _height(height)
{
    if (glCreateFramebuffers) {
        glCreateFramebuffers(1, &_name);
    } else if (glGenFramebuffers) {
        glGenFramebuffers(1, &_name);
        glBindFramebuffer(GL_FRAMEBUFFER, _name);
    }

    std::size_t num_color_attachments = 0;
    std::size_t num_depth_attachments = 0;
    std::size_t num_stencil_attachments = 0;
    std::size_t num_depth_stencil_attachments = 0;

    for (std::size_t ii = 0; ii < num_attachments; ++ii) {
        switch (attachments[ii]._type) {
            case attachment_type::color:
                _color_attachments.push_back(texture2dmultisample(samples, attachments[ii]._format, _width, _height, GL_FALSE));
                attach(
                    narrow_cast<GLenum>(GL_COLOR_ATTACHMENT0 + num_color_attachments),
                    GL_TEXTURE_2D_MULTISAMPLE,
                    _color_attachments[num_color_attachments].name(),
                    0);
                ++num_color_attachments;
                break;

            case attachment_type::depth:
                if (num_depth_attachments || num_depth_stencil_attachments) {
                    continue;
                }
                _depth_attachment = texture2dmultisample(samples, attachments[ii]._format, _width, _height, GL_FALSE);
                attach(
                    GL_DEPTH_ATTACHMENT,
                    GL_TEXTURE_2D_MULTISAMPLE,
                    _depth_attachment.name(),
                    0);
                ++num_depth_attachments;
                break;

            case attachment_type::stencil:
                if (num_stencil_attachments || num_depth_stencil_attachments) {
                    continue;
                }
                _stencil_attachment = texture2dmultisample(samples, attachments[ii]._format, _width, _height, GL_FALSE);
                attach(
                    GL_STENCIL_ATTACHMENT,
                    GL_TEXTURE_2D_MULTISAMPLE,
                    _stencil_attachment.name(),
                    0);
                ++num_stencil_attachments;
                break;

            case attachment_type::depth_stencil:
                if (num_depth_attachments || num_stencil_attachments) {
                    continue;
                }
                _depth_stencil_attachment = texture2dmultisample(samples, attachments[ii]._format, _width, _height, GL_FALSE);
                attach(
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    GL_TEXTURE_2D_MULTISAMPLE,
                    _depth_stencil_attachment.name(),
                    0);
                ++num_depth_stencil_attachments;
                break;
        }
    }
}

//------------------------------------------------------------------------------
void framebuffer::attach(GLenum attachment, GLenum target, GLuint texture, GLuint level)
{
    if (glNamedFramebufferTexture) {
        glNamedFramebufferTexture(_name, attachment, texture, level);
    } else if (glFramebufferTexture2D) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, target, texture, level);
    }
}

//------------------------------------------------------------------------------
framebuffer::~framebuffer()
{
    glDeleteFramebuffers(1, &_name);
}

//------------------------------------------------------------------------------
void framebuffer::read() const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _name);
}

//------------------------------------------------------------------------------
void framebuffer::draw() const
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _name);
}

//------------------------------------------------------------------------------
void framebuffer::read_buffer(GLenum buf) const
{
    if (glNamedFramebufferReadBuffer) {
        glNamedFramebufferReadBuffer(_name, buf);
    } else if (glBindFramebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _name);
        glReadBuffer(buf);
    }
}

//------------------------------------------------------------------------------
void framebuffer::draw_buffer(GLenum buf) const
{
    if (glNamedFramebufferDrawBuffer) {
        glNamedFramebufferDrawBuffer(_name, buf);
    } else if (glBindFramebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _name);
        glDrawBuffer(buf);
    }
}

//------------------------------------------------------------------------------
void framebuffer::draw_buffers(GLsizei n, GLenum const* bufs) const
{
    if (glNamedFramebufferDrawBuffers) {
        glNamedFramebufferDrawBuffers(_name, n, bufs);
    } else if (glBindFramebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _name);
        glDrawBuffers(n, bufs);
    }
}

//------------------------------------------------------------------------------
void framebuffer::blit(framebuffer const& src, GLbitfield mask, GLenum filter) const
{
    blit(src, 0, 0, src._width, src._height, 0, 0, _width, _height, mask, filter);
}

//------------------------------------------------------------------------------
void framebuffer::blit(framebuffer const& src, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const
{
    if (glBlitNamedFramebuffer) {
        glBlitNamedFramebuffer(src._name, _name, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    } else if (glBlitFramebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, src._name);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _name);
        glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    }
}

} // namespace gl
} // namespace render
