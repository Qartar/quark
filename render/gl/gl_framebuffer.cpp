// gl_framebuffer.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "gl/gl_include.h"
#include "gl/gl_framebuffer.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

framebuffer::PFNGLBINDRENDERBUFFER framebuffer::glBindRenderbuffer = nullptr;
framebuffer::PFNGLDELETERENDERBUFFERS framebuffer::glDeleteRenderbuffers = nullptr;
framebuffer::PFNGLGENRENDERBUFFERS framebuffer::glGenRenderbuffers = nullptr;
framebuffer::PFNGLRENDERBUFFERSTORAGE framebuffer::glRenderbufferStorage = nullptr;
framebuffer::PFNGLRENDERBUFFERSTORAGEMULTISAMPLE framebuffer::glRenderbufferStorageMultisample = nullptr;
framebuffer::PFNGLBINDFRAMEBUFFER framebuffer::glBindFramebuffer = nullptr;
framebuffer::PFNGLDELETEFRAMEBUFFERS framebuffer::glDeleteFramebuffers = nullptr;
framebuffer::PFNGLGENFRAMEBUFFERS framebuffer::glGenFramebuffers = nullptr;
framebuffer::PFNGLFRAMEBUFFERRENDERBUFFER framebuffer::glFramebufferRenderbuffer = nullptr;
framebuffer::PFNGLFRAMEBUFFERTEXTURE2D framebuffer::glFramebufferTexture2D = nullptr;
framebuffer::PFNGLBLITFRAMEBUFFER framebuffer::glBlitFramebuffer = nullptr;

framebuffer::PFNGLDRAWBUFFERS framebuffer::glDrawBuffers = nullptr;

//------------------------------------------------------------------------------
void framebuffer::init()
{
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFER )wglGetProcAddress("glBindRenderbuffer");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERS )wglGetProcAddress("glDeleteRenderbuffers");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERS )wglGetProcAddress("glGenRenderbuffers");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGE )wglGetProcAddress("glRenderbufferStorage");
    glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLE )wglGetProcAddress("glRenderbufferStorageMultisample");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFER )wglGetProcAddress("glBindFramebuffer");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERS )wglGetProcAddress("glDeleteFramebuffers");
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERS )wglGetProcAddress("glGenFramebuffers");
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFER )wglGetProcAddress("glFramebufferRenderbuffer");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2D )wglGetProcAddress("glFramebufferTexture2D");
    glBlitFramebuffer = (PFNGLBLITFRAMEBUFFER )wglGetProcAddress("glBlitFramebuffer");

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
    glGenFramebuffers(1, &_name);
    glBindFramebuffer(GL_FRAMEBUFFER, _name);

    std::size_t num_color_attachments = 0;
    std::size_t num_depth_attachments = 0;
    std::size_t num_stencil_attachments = 0;
    std::size_t num_depth_stencil_attachments = 0;

    for (std::size_t ii = 0; ii < num_attachments; ++ii) {
        switch (attachments[ii]._type) {
            case attachment_type::color:
                _color_attachments.push_back(texture2d(1, attachments[ii]._format, _width, _height));
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
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
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
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
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
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
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
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
    glGenFramebuffers(1, &_name);
    glBindFramebuffer(GL_FRAMEBUFFER, _name);

    std::size_t num_color_attachments = 0;
    std::size_t num_depth_attachments = 0;
    std::size_t num_stencil_attachments = 0;
    std::size_t num_depth_stencil_attachments = 0;

    for (std::size_t ii = 0; ii < num_attachments; ++ii) {
        switch (attachments[ii]._type) {
            case attachment_type::color:
                _color_attachments.push_back(texture2dmultisample(samples, attachments[ii]._format, _width, _height, GL_FALSE));
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
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
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
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
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
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
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
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
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _name);
    glReadBuffer(buf);
}

//------------------------------------------------------------------------------
void framebuffer::draw_buffer(GLenum buf) const
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _name);
    glDrawBuffer(buf);
}

//------------------------------------------------------------------------------
void framebuffer::draw_buffers(GLsizei n, GLenum const* bufs) const
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _name);
    glDrawBuffers(n, bufs);
}

//------------------------------------------------------------------------------
void framebuffer::blit(framebuffer const& src, GLbitfield mask, GLenum filter) const
{
    blit(src, 0, 0, src._width, src._height, 0, 0, _width, _height, mask, filter);
}

//------------------------------------------------------------------------------
void framebuffer::blit(framebuffer const& src, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const
{
    src.read();
    draw();
    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

} // namespace gl
} // namespace render
