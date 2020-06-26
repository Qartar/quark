// gl_shader.h
//

#pragma once

#include "cm_string.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

//------------------------------------------------------------------------------
enum class shader_stage
{
    vertex,
    fragment,
};

//------------------------------------------------------------------------------
class shader
{
public:
    //! Initialize function pointers
    static void init();

    shader();
    shader(shader&& s) noexcept;
    shader& operator=(shader&& s) noexcept;
    ~shader();

    shader(shader_stage stage, string::view source);

    explicit operator bool() const { return _shader != 0; }

    bool compile_status(string::buffer& info_log) const;

protected:
    friend class program;
    shader_stage _stage;
    GLuint _shader;

protected:
    typedef GLuint (APIENTRY* PFNGLCREATESHADER)(GLenum shaderType);
    typedef void (APIENTRY* PFNGLGETSHADERIV)(GLuint shader, GLenum pname, GLint* params);
    typedef void (APIENTRY* PFNGLGETSHADERINFOLOG)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
    typedef void (APIENTRY* PFNGLSHADERSOURCE)(GLuint shader, GLsizei count, GLchar const** strings, GLint const* length);
    typedef void (APIENTRY* PFNGLCOMPILESHADER)(GLuint shader);
    typedef void (APIENTRY* PFNGLDELETESHADER)(GLuint shader);

    static PFNGLCREATESHADER glCreateShader;
    static PFNGLGETSHADERIV glGetShaderiv;
    static PFNGLGETSHADERINFOLOG glGetShaderInfoLog;
    static PFNGLSHADERSOURCE glShaderSource;
    static PFNGLCOMPILESHADER glCompileShader;
    static PFNGLDELETESHADER glDeleteShader;
};

//------------------------------------------------------------------------------
class program
{
public:
    //! Initialize function pointers
    static void init();

    program();
    program(program&& p) noexcept;
    program& operator=(program&& p) noexcept;
    ~program();

    program(shader const& vertex, shader const& fragment);

    bool link_status(string::buffer& info_log) const;
    bool validate_status(string::buffer& info_log) const;

    void use() const;

protected:
    GLuint _program;

protected:
    typedef GLuint (APIENTRY* PFNGLCREATEPROGRAM)();
    typedef void (APIENTRY* PFNGLUSEPROGRAM)(GLuint program);
    typedef void (APIENTRY* PFNGLGETPROGRAMIV)(GLuint program, GLenum pname, GLint* params);
    typedef void (APIENTRY* PFNGLGETPROGRAMINFOLOG)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
    typedef void (APIENTRY* PFNGLDELETEPROGRAM)(GLuint program);
    typedef void (APIENTRY* PFNGLATTACHSHADER)(GLuint program, GLuint shader);
    typedef void (APIENTRY* PFNGLDETACHSHADER)(GLuint program, GLuint shader);
    typedef void (APIENTRY* PFNGLLINKPROGRAM)(GLuint program);

    static PFNGLCREATEPROGRAM glCreateProgram;
    static PFNGLUSEPROGRAM glUseProgram;
    static PFNGLGETPROGRAMIV glGetProgramiv;
    static PFNGLGETPROGRAMINFOLOG glGetProgramInfoLog;
    static PFNGLDELETEPROGRAM glDeleteProgram;
    static PFNGLATTACHSHADER glAttachShader;
    static PFNGLDETACHSHADER glDetachShader;
    static PFNGLLINKPROGRAM glLinkProgram;
};

} // namespace gl
} // namespace render
