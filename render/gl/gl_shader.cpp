// gl_shader.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "gl/gl_include.h"
#include "gl/gl_shader.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
namespace gl {

//------------------------------------------------------------------------------
namespace {

constexpr GLenum gl_shader_type(shader_stage stage)
{
    switch (stage) {
        case shader_stage::vertex:
            return GL_VERTEX_SHADER;
        case shader_stage::fragment:
            return GL_FRAGMENT_SHADER;
        default:
            ASSUME(false);
    }
}

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
shader::PFNGLCREATESHADER shader::glCreateShader = nullptr;
shader::PFNGLGETSHADERIV shader::glGetShaderiv = nullptr;
shader::PFNGLGETSHADERINFOLOG shader::glGetShaderInfoLog = nullptr;
shader::PFNGLSHADERSOURCE shader::glShaderSource = nullptr;
shader::PFNGLCOMPILESHADER shader::glCompileShader = nullptr;
shader::PFNGLDELETESHADER shader::glDeleteShader = nullptr;

//------------------------------------------------------------------------------
void shader::init(get_proc_address_t get_proc_address)
{
    glCreateShader = (PFNGLCREATESHADER)get_proc_address("glCreateShader");
    glGetShaderiv = (PFNGLGETSHADERIV)get_proc_address("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOG)get_proc_address("glGetShaderInfoLog");
    glShaderSource = (PFNGLSHADERSOURCE)get_proc_address("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADER)get_proc_address("glCompileShader");
    glDeleteShader = (PFNGLDELETESHADER)get_proc_address("glDeleteShader");
}

//------------------------------------------------------------------------------
shader::shader()
    : _stage{}
    , _shader(0)
{}

//------------------------------------------------------------------------------
shader::shader(shader&& s) noexcept
    : _stage(s._stage)
    , _shader(s._shader)
{
    s._shader = 0;
}

//------------------------------------------------------------------------------
shader& shader::operator=(shader&& s) noexcept
{
    // "A value of 0 for shader will be silently ignored"
    glDeleteShader(_shader);
    _stage = s._stage;
    _shader = s._shader;
    s._shader = 0;
    return *this;
}

//------------------------------------------------------------------------------
shader::~shader()
{
    // "A value of 0 for shader will be silently ignored"
    glDeleteShader(_shader);
}

//------------------------------------------------------------------------------
shader::shader(shader_stage stage, string::view source)
    : _stage(stage)
    , _shader(glCreateShader(gl_shader_type(stage)))
{
    GLchar const* strings = source.begin();
    GLint length = narrow_cast<GLint>(source.length());
    glShaderSource(_shader, 1, &strings, &length);
    glCompileShader(_shader);
}

//------------------------------------------------------------------------------
bool shader::compile_status(string::buffer& info_log) const
{
    GLint status;
    glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
    GLint log_length = 0;
    glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length) {
        info_log.resize(log_length - 1); // log_length includes null terminator
        glGetShaderInfoLog(_shader, log_length, nullptr, info_log.data());
    } else {
        info_log = {};
    }
    return status == GL_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
program::PFNGLCREATEPROGRAM program::glCreateProgram = nullptr;
program::PFNGLUSEPROGRAM program::glUseProgram = nullptr;
program::PFNGLGETPROGRAMIV program::glGetProgramiv = nullptr;
program::PFNGLGETPROGRAMINFOLOG program::glGetProgramInfoLog = nullptr;
program::PFNGLDELETEPROGRAM program::glDeleteProgram = nullptr;
program::PFNGLATTACHSHADER program::glAttachShader = nullptr;
program::PFNGLDETACHSHADER program::glDetachShader = nullptr;
program::PFNGLLINKPROGRAM program::glLinkProgram = nullptr;

//------------------------------------------------------------------------------
void program::init(get_proc_address_t get_proc_address)
{
    glCreateProgram = (PFNGLCREATEPROGRAM)get_proc_address("glCreateProgram");
    glUseProgram = (PFNGLUSEPROGRAM)get_proc_address("glUseProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIV)get_proc_address("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOG)get_proc_address("glGetProgramInfoLog");
    glDeleteProgram = (PFNGLDELETEPROGRAM)get_proc_address("glDeleteProgram");
    glAttachShader = (PFNGLATTACHSHADER)get_proc_address("glAttachShader");
    glDetachShader = (PFNGLDETACHSHADER)get_proc_address("glDetachShader");
    glLinkProgram = (PFNGLLINKPROGRAM)get_proc_address("glLinkProgram");
}

//------------------------------------------------------------------------------
program::program()
    : _program(0)
{}

//------------------------------------------------------------------------------
program::program(program&& p) noexcept
    : _program(p._program)
{
    p._program = 0;
}

//------------------------------------------------------------------------------
program& program::operator=(program&& p) noexcept
{
    // "A value of 0 for program will be silently ignored"
    glDeleteProgram(_program);
    _program = p._program;
    p._program = 0;
    return *this;
}

//------------------------------------------------------------------------------
program::~program()
{
    // "A value of 0 for program will be silently ignored"
    glDeleteProgram(_program);
}

//------------------------------------------------------------------------------
program::program(shader const& vertex, shader const& fragment)
    : _program(glCreateProgram())
{
    glAttachShader(_program, vertex._shader);
    glAttachShader(_program, fragment._shader);
    glLinkProgram(_program);
}

//------------------------------------------------------------------------------
void program::use() const
{
    glUseProgram(_program);
}

//------------------------------------------------------------------------------
bool program::link_status(string::buffer& info_log) const
{
    GLint status = 0;
    glGetProgramiv(_program, GL_LINK_STATUS, &status);
    GLint log_length = 0;
    glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length) {
        info_log.resize(log_length - 1); // log_length includes null terminator
        glGetProgramInfoLog(_program, log_length, nullptr, info_log.data());
    } else {
        info_log = {};
    }
    return status == GL_TRUE;
}

//------------------------------------------------------------------------------
bool program::validate_status(string::buffer& info_log) const
{
    GLint status;
    glGetProgramiv(_program, GL_VALIDATE_STATUS, &status);
    GLint log_length = 0;
    glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length) {
        info_log.resize(log_length - 1); // log_length includes null terminator
        glGetProgramInfoLog(_program, log_length, nullptr, info_log.data());
    } else {
        info_log = {};
    }
    return status == GL_TRUE;
}

} // namespace gl
} // namespace render
