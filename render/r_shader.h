// r_shader.h
//

#pragma once

#include "cm_filesystem.h"
#include "cm_string.h"
#include "gl/gl_shader.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
class shader
{
public:
    shader(string::view name, string::view vertex_shader, string::view fragment_shader);
    result reload();

    string::view name() const { return _name; }
    gl::program const& program() const { return _program; }

protected:
    string::buffer _name;

    string::buffer _vertex_shader_filename;
    file::time _vertex_shader_filetime;
    gl::shader _vertex_shader;

    string::buffer _fragment_shader_filename;
    file::time _fragment_shader_filetime;
    gl::shader _fragment_shader;

    gl::program _program;

protected:
    result try_compile_shader(gl::shader_stage stage, string::buffer const& filename, gl::shader& shader) const;
    result try_link_program(gl::shader const& vertex_shader, gl::shader const& fragment_shader, gl::program& program) const;
};

} // namespace render
