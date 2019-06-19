// r_shader.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "r_shader.h"
#include "cm_filesystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

namespace {

//------------------------------------------------------------------------------
constexpr string::literal gl_shader_stage_name(gl::shader_stage stage)
{
    switch (stage) {
        case gl::shader_stage::vertex: return "vertex";
        case gl::shader_stage::fragment: return "fragment";
    }
    ASSUME(false);
}

} // anonymous namespace

//------------------------------------------------------------------------------
shader::shader(string::view name, string::view vertex_shader, string::view fragment_shader)
    : _name(name)
    , _vertex_shader_filename(vertex_shader)
    , _vertex_shader_filetime{}
    , _fragment_shader_filename(fragment_shader)
    , _fragment_shader_filetime{}
{
    reload();
}

//------------------------------------------------------------------------------
result shader::reload()
{
    result res = result::none;

    file::time new_vertex_shader_filetime = file::modified_time(_vertex_shader_filename);
    gl::shader new_vertex_shader;
    if (_vertex_shader_filetime < new_vertex_shader_filetime) {
        result compile_result = try_compile_shader(
            gl::shader_stage::vertex,
            _vertex_shader_filename,
            new_vertex_shader);

        if (failed(compile_result)) {
            res = compile_result;
        }
    }

    file::time new_fragment_shader_filetime = file::modified_time(_fragment_shader_filename);
    gl::shader new_fragment_shader;
    if (_fragment_shader_filetime < new_fragment_shader_filetime) {
        result compile_result = try_compile_shader(
            gl::shader_stage::fragment,
            _fragment_shader_filename,
            new_fragment_shader);

        if (failed(compile_result)) {
            res = compile_result;
        }
    }

    if (!failed(res) && (new_vertex_shader || new_fragment_shader)) {
        gl::program new_program;

        result link_result = try_link_program(
            new_vertex_shader ? new_vertex_shader : _vertex_shader,
            new_fragment_shader ? new_fragment_shader : _fragment_shader,
            new_program);

        if (failed(link_result)) {
            res = link_result;
        } else {
            if (new_vertex_shader) {
                _vertex_shader_filetime = new_vertex_shader_filetime;
                _vertex_shader = std::move(new_vertex_shader);
            }

            if (new_fragment_shader) {
                _fragment_shader_filetime = new_fragment_shader_filetime;
                _fragment_shader = std::move(new_fragment_shader);
            }

            _program = std::move(new_program);
            res = result::success;
        }
    }

    return res;
}

//------------------------------------------------------------------------------
result shader::try_compile_shader(
    gl::shader_stage stage,
    string::buffer const& filename,
    gl::shader& shader) const
{
    file::buffer source = file::read(filename);
    string::view source_text(reinterpret_cast<char const*>(source.data()),
                             reinterpret_cast<char const*>(source.data() + source.size()));
    shader = gl::shader(stage, source_text);

    string::buffer info_log;
    if (!shader.compile_status(info_log)) {
        if (info_log.length()) {
            log::error("%s shader '^fff%s^xxx' failed to compile with errors:\n%s", gl_shader_stage_name(stage).c_str(), filename.c_str(), info_log.c_str());
        } else {
            log::error("%s shader '^fff%s^xxx' failed to compile\n", gl_shader_stage_name(stage).c_str(), filename.c_str());
        }
        return result::failure;
    } else {
        if (info_log.length()) {
            log::warning("%s shader '^fff%s^xxx' compiled with warnings:\n%s", gl_shader_stage_name(stage).c_str(), filename.c_str(), info_log.c_str());
        }
        return result::success;
    }
}

//------------------------------------------------------------------------------
result shader::try_link_program(
    gl::shader const& vertex_shader,
    gl::shader const& fragment_shader,
    gl::program& program) const
{
    program = gl::program(vertex_shader, fragment_shader);

    string::buffer info_log;
    if (!program.link_status(info_log)) {
        if (info_log.length()) {
            log::error("shader program '^fff%s^xxx' failed to link with errors:\n%s", _name.c_str(), info_log.c_str());
        } else {
            log::error("shader program '^fff%s^xxx' failed to link\n", _name.c_str());
        }
        return result::failure;
    } else {
        if (info_log.length()) {
            log::warning("shader program '^fff%s^xxx' linked with warnings:\n%s", _name.c_str(), info_log.c_str());
        }
        return result::success;
    }
}

//------------------------------------------------------------------------------
shader const* system::load_shader(
    string::view name,
    string::view vertex_shader,
    string::view fragment_shader)
{
    for (std::size_t ii = 0, sz = _shaders.size(); ii < sz; ++ii) {
        if (name == _shaders[ii]->name()) {
            return _shaders[ii].get();
        }
    }

    _shaders.push_back(std::make_unique<shader>(name, vertex_shader, fragment_shader));
    return _shaders.back().get();
}

//------------------------------------------------------------------------------
void system::command_list_shaders(parser::text const&) const
{
    for (auto& shader : _shaders) {
        log::message("    %.*s\n", shader->name().length(), shader->name().begin());
    }
}

//------------------------------------------------------------------------------
void system::command_reload_shaders(parser::text const&) const
{
    time_value t = time_value::current();
    std::size_t num_reloaded = 0;
    std::vector<string::view> failed;
    for (auto& shader : _shaders) {
        result r = shader->reload();
        if (succeeded(r)) {
            ++num_reloaded;
        } else if (::failed(r)) {
            failed.push_back(shader->name());
        }
    }
    time_delta dt = (time_value::current() - t);
    if (!failed.size()) {
        log::message("...reloaded %zu shaders in %zd ms\n", num_reloaded, dt.to_milliseconds());
    } else {
        log::message("...reloaded %zu shaders in %zd ms, ^f00%zu failed^xxx\n", num_reloaded + failed.size(), dt.to_milliseconds(), failed.size());
        for (string::view shader_name : failed) {
            log::message("    ^f00%.*s^xxx\n", shader_name.length(), shader_name.begin());
        }
    }
}

} // namespace render
