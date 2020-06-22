// gl_texture.h
//

#pragma once

#include "gl/gl_types.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

//------------------------------------------------------------------------------
class texture
{
public:
    //! Initialize function pointers
    static void init();

    texture();
    texture(texture&& t) noexcept;
    texture& operator=(texture&& t) noexcept;
    ~texture();

    GLuint name() const { return _name; }

    void bind(GLuint textureunit = 0) const;

protected:
    GLuint _name;
    GLenum _target;
    GLsizei _levels;

protected:
    texture(GLenum target, GLsizei levels);

protected:
    // OpenGL 1.3
    typedef void (APIENTRY* PFNGLACTIVETEXTURE)(GLenum texture);

    static PFNGLACTIVETEXTURE glActiveTexture;

    // ARB_texture_storage
    typedef void (APIENTRY* PFNGLTEXSTORAGE2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    typedef void (APIENTRY* PFNGLTEXSTORAGE2DMULTISAMPLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);

    static PFNGLTEXSTORAGE2D glTexStorage2D;
    static PFNGLTEXSTORAGE2DMULTISAMPLE glTexStorage2DMultisample;

    // ARB_direct_state_access
    typedef void (APIENTRY* PFNGLCREATETEXTURES)(GLenum target, GLsizei n, GLuint* textures);
    typedef void (APIENTRY* PFNGLTEXTURESUBIMAGE2D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, void const* pixels);
    typedef void (APIENTRY* PFNGLBINDTEXTUREUNIT)(GLuint unit, GLuint texture);
    typedef void (APIENTRY* PFNGLTEXTURESTORAGE2D)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    typedef void (APIENTRY* PFNGLTEXTURESTORAGE2DMULTISAMPLE)(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);

    static PFNGLCREATETEXTURES glCreateTextures;
    static PFNGLTEXTURESUBIMAGE2D glTextureSubImage2D;
    static PFNGLBINDTEXTUREUNIT glBindTextureUnit;
    static PFNGLTEXTURESTORAGE2D glTextureStorage2D;
    static PFNGLTEXTURESTORAGE2DMULTISAMPLE glTextureStorage2DMultisample;
};

//------------------------------------------------------------------------------
class texture2d : public texture
{
public:
    texture2d()
        : texture() {}
    texture2d(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);

    void upload(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, void const* pixels);
};

//------------------------------------------------------------------------------
class texture2dmultisample : public texture
{
public:
    texture2dmultisample()
        : texture() {}
    texture2dmultisample(GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
};

} // namespace gl
} // namespace render
