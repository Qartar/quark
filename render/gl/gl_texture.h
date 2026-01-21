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

    void parameter(GLenum pname, GLfloat param);
    void parameter(GLenum pname, GLfloat const* params);
    void parameter(GLenum pname, GLint param);
    void parameter(GLenum pname, GLint const* params);

protected:
    GLuint _name;
    GLenum _target;
    GLsizei _levels;

protected:
    texture(GLenum target, GLsizei levels);

protected:
    // OpenGL 1.3
    using PFNGLACTIVETEXTURE = void (APIENTRY*)(GLenum texture);

    static PFNGLACTIVETEXTURE glActiveTexture;

    // ARB_texture_storage
    using PFNGLTEXSTORAGE2D = void (APIENTRY*)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    using PFNGLTEXSTORAGE2DMULTISAMPLE = void (APIENTRY*)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);

    static PFNGLTEXSTORAGE2D glTexStorage2D;
    static PFNGLTEXSTORAGE2DMULTISAMPLE glTexStorage2DMultisample;

    // ARB_direct_state_access
    using PFNGLCREATETEXTURES = void (APIENTRY*)(GLenum target, GLsizei n, GLuint* textures);
    using PFNGLTEXTURESUBIMAGE2D = void (APIENTRY*)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, void const* pixels);
    using PFNGLBINDTEXTUREUNIT = void (APIENTRY*)(GLuint unit, GLuint texture);
    using PFNGLTEXTURESTORAGE2D = void (APIENTRY*)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    using PFNGLTEXTURESTORAGE2DMULTISAMPLE = void (APIENTRY*)(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
    using PFNGLTEXTUREPARAMETERF = void (APIENTRY*)(GLuint texture, GLenum pname, GLfloat param);
    using PFNGLTEXTUREPARAMETERFV = void (APIENTRY*)(GLuint texture, GLenum pname, GLfloat const* params);
    using PFNGLTEXTUREPARAMETERI = void (APIENTRY*)(GLuint texture, GLenum pname, GLint param);
    using PFNGLTEXTUREPARAMETERIV = void (APIENTRY*)(GLuint texture, GLenum pname, GLint const* params);

    static PFNGLCREATETEXTURES glCreateTextures;
    static PFNGLTEXTURESUBIMAGE2D glTextureSubImage2D;
    static PFNGLBINDTEXTUREUNIT glBindTextureUnit;
    static PFNGLTEXTURESTORAGE2D glTextureStorage2D;
    static PFNGLTEXTURESTORAGE2DMULTISAMPLE glTextureStorage2DMultisample;
    static PFNGLTEXTUREPARAMETERF glTextureParameterf;
    static PFNGLTEXTUREPARAMETERFV glTextureParameterfv;
    static PFNGLTEXTUREPARAMETERI glTextureParameteri;
    static PFNGLTEXTUREPARAMETERIV glTextureParameteriv;
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
