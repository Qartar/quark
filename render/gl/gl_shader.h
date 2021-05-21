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
    static void init(get_proc_address_t get_proc_address);

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
    using PFNGLCREATESHADER = GLuint (APIENTRY*)(GLenum shaderType);
    using PFNGLGETSHADERIV = void (APIENTRY*)(GLuint shader, GLenum pname, GLint* params);
    using PFNGLGETSHADERINFOLOG = void (APIENTRY*)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
    using PFNGLSHADERSOURCE = void (APIENTRY*)(GLuint shader, GLsizei count, GLchar const** strings, GLint const* length);
    using PFNGLCOMPILESHADER = void (APIENTRY*)(GLuint shader);
    using PFNGLDELETESHADER = void (APIENTRY*)(GLuint shader);

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
    static void init(get_proc_address_t get_proc_address);

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
    using PFNGLCREATEPROGRAM = GLuint (APIENTRY*)();
    using PFNGLUSEPROGRAM = void (APIENTRY*)(GLuint program);
    using PFNGLGETPROGRAMIV = void (APIENTRY*)(GLuint program, GLenum pname, GLint* params);
    using PFNGLGETPROGRAMINFOLOG = void (APIENTRY*)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
    using PFNGLDELETEPROGRAM = void (APIENTRY*)(GLuint program);
    using PFNGLATTACHSHADER = void (APIENTRY*)(GLuint program, GLuint shader);
    using PFNGLDETACHSHADER = void (APIENTRY*)(GLuint program, GLuint shader);
    using PFNGLLINKPROGRAM = void (APIENTRY*)(GLuint program);

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
