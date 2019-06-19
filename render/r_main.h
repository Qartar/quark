// r_main.h
//

#pragma once

#include "cm_config.h"
#include "cm_string.h"
#include "cm_time.h"

#include "gl/gl_framebuffer.h"
#include "gl/gl_types.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

class window;
class shader;
class image;
class font;

//------------------------------------------------------------------------------
struct view
{
    vec2 origin; //!< center
    float angle;
    bool raster; //!< use raster-coordinates, i.e. origin at top-left
    vec2 size;
    rect viewport;
};

//------------------------------------------------------------------------------
class system
{
public:
    system(render::window* window);
    ~system();

    result init();
    void shutdown();

    void begin_frame();
    void end_frame();

    void resize(vec2i size);
    render::window const* window() const { return _window; }

    render::view const& view() const { return _view; }

    // Font Interface (r_font.cpp)

    render::font const* load_font(string::view name, int nSize);

    //  Image Interface (r_image.cpp)
    render::image const* load_image(string::view name);
    void draw_image(render::image const* img, vec2 org, vec2 sz, color4 color = color4(1,1,1,1));

    render::shader const* load_shader(string::view name, string::view vertex_shader_filename, string::view fragment_shader_filename);

    // Drawing Functions (r_draw.cpp)

    void draw_string(string::view string, vec2 position, color4 color);
    vec2 string_size(string::view string) const;

    void draw_monospace(string::view string, vec2 position, color4 color);
    vec2 monospace_size(string::view string) const;

    void draw_line(vec2 start, vec2 end, color4 start_color, color4 end_color);
    void draw_line(float width, vec2 start, vec2 end, color4 core_color, color4 edge_color) { draw_line(width, start, end, core_color, core_color, edge_color, edge_color); }
    void draw_line(float width, vec2 start, vec2 end, color4 start_color, color4 end_color, color4 start_edge_color, color4 end_edge_color);
    void draw_arc(vec2 center, float radius, float width, float min_angle, float max_angle, color4 color);
    void draw_box(vec2 size, vec2 position, color4 color);
    void draw_triangles(vec2 const* position, color4 const* color, int const* indices, std::size_t num_indices);
    void draw_particles(time_value time, render::particle const* particles, std::size_t num_particles);
    void draw_model(render::model const* model, mat3 transform, color4 color);
    void draw_starfield(vec2 streak_vector = vec2_zero);

    void set_view(render::view const& view);

private:

    // More font stuff (r_font.cpp)

    static constexpr int _font_size = 48;
    //! Global scaling factor applied to font rendering to give consistent
    //! text size regardless of window size and display scaling.
    float _font_scale;

    std::unique_ptr<render::font> _default_font;
    std::unique_ptr<render::font> _monospace_font;

    std::vector<std::unique_ptr<render::font>> _fonts;

    std::vector<std::unique_ptr<render::image>> _images;

    std::vector<std::unique_ptr<render::shader>> _shaders;

    console_command _command_list_shaders;
    console_command _command_reload_shaders;
    void command_list_shaders(parser::text const& args) const;
    void command_reload_shaders(parser::text const& args) const;

    // Internal stuff

    config::integer _framebuffer_width;
    config::integer _framebuffer_height;
    config::scalar _framebuffer_scale;
    config::integer _framebuffer_samples;

    gl::framebuffer _framebuffer;

    render::window* _window;

    void create_default_font();
    void set_default_state();
    void create_framebuffer(vec2i size, int samples);
    void destroy_framebuffer();

    render::view _view;
    bounds _view_bounds;

    config::boolean _draw_tris;

    config::boolean _graph;
    struct timing {
        time_value begin_frame;
        time_value end_frame;
        time_value swap_buffer;
    };
    std::vector<timing> _timers;
    std::size_t _timer_index;

    void draw_timers() const;

    std::vector<vec3> _starfield_points;
    std::vector<color3> _starfield_colors;

    float _costbl[360];
    float _sintbl[360];

private:

    // additional opengl bindings
    using PFNGLBLENDCOLOR = void (APIENTRY*)(GLfloat red, GLfloat greed, GLfloat blue, GLfloat alpha);

    PFNGLBLENDCOLOR glBlendColor = NULL;
};

} // namespace render
