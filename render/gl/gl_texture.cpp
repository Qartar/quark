// gl_texture.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "gl/gl_include.h"
#include "gl/gl_texture.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

texture::PFNGLACTIVETEXTURE texture::glActiveTexture = nullptr;
texture::PFNGLTEXSTORAGE2D texture::glTexStorage2D = nullptr;
texture::PFNGLTEXSTORAGE2DMULTISAMPLE texture::glTexStorage2DMultisample = nullptr;
texture::PFNGLCREATETEXTURES texture::glCreateTextures = nullptr;
texture::PFNGLTEXTURESUBIMAGE2D texture::glTextureSubImage2D = nullptr;
texture::PFNGLBINDTEXTUREUNIT texture::glBindTextureUnit = nullptr;
texture::PFNGLTEXTURESTORAGE2D texture::glTextureStorage2D = nullptr;
texture::PFNGLTEXTURESTORAGE2DMULTISAMPLE texture::glTextureStorage2DMultisample = nullptr;

////////////////////////////////////////////////////////////////////////////////
void texture::init(get_proc_address_t get_proc_address)
{
    // OpenGL 1.3
    glActiveTexture = (PFNGLACTIVETEXTURE)get_proc_address("glActiveTexture");
    // ARB_texture_storage
    glTexStorage2D = (PFNGLTEXSTORAGE2D)get_proc_address("glTexStorage2D");
    glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLE)get_proc_address("glTexStorage2DMultisample");
    // ARB_direct_state_access
    glCreateTextures = (PFNGLCREATETEXTURES)get_proc_address("glCreateTextures");
    glTextureSubImage2D = (PFNGLTEXTURESUBIMAGE2D)get_proc_address("glTextureSubImage2D");
    glBindTextureUnit = (PFNGLBINDTEXTUREUNIT)get_proc_address("glBindTextureUnit");
    glTextureStorage2D = (PFNGLTEXTURESTORAGE2D)get_proc_address("glTextureStorage2D");
    glTextureStorage2DMultisample = (PFNGLTEXTURESTORAGE2DMULTISAMPLE)get_proc_address("glTextureStorage2DMultisample");
}

//------------------------------------------------------------------------------
texture::texture()
    : _name(0)
    , _target(0)
    , _levels(0)
{}

//------------------------------------------------------------------------------
texture::texture(GLenum target, GLsizei levels)
    : _name(0)
    , _target(target)
    , _levels(levels)
{
    if (glCreateTextures) {
        glCreateTextures(_target, 1, &_name);
    } else {
        glGenTextures(1, &_name);
        glBindTexture(_target, _name);
    }
}

//------------------------------------------------------------------------------
texture::texture(texture&& t) noexcept
    : _name(t._name)
    , _target(t._target)
    , _levels(t._levels)
{
    t._name = 0;
}

//------------------------------------------------------------------------------
texture& texture::operator=(texture&& t) noexcept
{
    // "glDeleteTextures silently ignores 0's"
    glDeleteTextures(1, &_name);
    _name = t._name;
    _target = t._target;
    _levels = t._levels;
    t._name = 0;
    return *this;
}

//------------------------------------------------------------------------------
texture::~texture()
{
    // "glDeleteTextures silently ignores 0's"
    glDeleteTextures(1, &_name);
}

//------------------------------------------------------------------------------
void texture::bind(GLuint textureunit) const
{
    if (glBindTextureUnit) {
        glBindTextureUnit(textureunit, _name);
    } else if (glActiveTexture) {
        glActiveTexture(GL_TEXTURE0 + textureunit);
        glBindTexture(_target, _name);
    }
}

////////////////////////////////////////////////////////////////////////////////
texture2d::texture2d(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
    : texture(GL_TEXTURE_2D, levels)
{
    if (glTextureStorage2D) {
        glTextureStorage2D(_name, levels, internalformat, width, height);
    } else if (glTexStorage2D) {
        glTexStorage2D(GL_TEXTURE_2D, levels, internalformat, width, height);
    }
}

//------------------------------------------------------------------------------
void texture2d::upload(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, void const* pixels)
{
    if (glTextureSubImage2D) {
        glTextureSubImage2D(_name, level, xoffset, yoffset, width, height, format, type, pixels);
    } else {
        glBindTexture(_target, _name);
        glTexSubImage2D(_target, level, xoffset, yoffset, width, height, format, type, pixels);
    }
}

////////////////////////////////////////////////////////////////////////////////
texture2dmultisample::texture2dmultisample(GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
    : texture(GL_TEXTURE_2D_MULTISAMPLE, 1)
{
    if (glTextureStorage2DMultisample) {
        glTextureStorage2DMultisample(_name, samples, internalformat, width, height, fixedsamplelocations);
    } else if (glTexStorage2DMultisample) {
        glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalformat, width, height, fixedsamplelocations);
    }
}

} // namespace gl
} // namespace render
