// r_main.h
//

#pragma once

#include "cm_config.h"
#include "cm_string.h"
#include "cm_time.h"

#ifndef _WINDOWS_
typedef struct HFONT__* HFONT;
typedef struct HBITMAP__* HBITMAP;
#endif // _WINDOWS_

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef float GLfloat;

#define GL_FRAMEBUFFER                  0x8D40
#define GL_READ_FRAMEBUFFER             0x8CA8
#define GL_DRAW_FRAMEBUFFER             0x8CA9
#define GL_RENDERBUFFER                 0x8D41
#define GL_MAX_SAMPLES                  0x8D57
#define GL_COLOR_ATTACHMENT0            0x8CE0
#define GL_DEPTH_STENCIL_ATTACHMENT     0x821A
#define GL_DEPTH24_STENCIL8             0x88F0

#define GL_CONSTANT_COLOR               0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR     0x8002
#define GL_CONSTANT_ALPHA               0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA     0x8004

////////////////////////////////////////////////////////////////////////////////
namespace render {

class window;

//------------------------------------------------------------------------------
class font
{
public:
    font(string::view name, int size);
    ~font();

    bool compare(string::view name, int size) const;
    void draw(string::view string, vec2 position, color4 color, vec2 scale) const;
    vec2 size(string::view string, vec2 scale) const;

private:
    constexpr static int kNumChars = 256;

    string::buffer _name;
    int _size;

    HFONT _handle;
    unsigned int _list_base;

    short _char_width[kNumChars];

    static HFONT _system_font;
    static HFONT _active_font;
};

//------------------------------------------------------------------------------
class image
{
public:
    image(string::view name);
    ~image();

    string::view name() const { return _name; }
    unsigned int texnum() const { return _texnum; }
    int width() const { return _width; }
    int height() const { return _height; }

protected:
    string::buffer _name;
    unsigned int _texnum;
    int _width;
    int _height;

protected:
    HBITMAP load_resource(string::view name) const;
    HBITMAP load_file(string::view name) const;

    bool upload(HBITMAP bitmap);
};

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

    // Drawing Functions (r_draw.cpp)

    void draw_string(string::view string, vec2 position, color4 color);
    vec2 string_size(string::view string) const;

    void draw_monospace(string::view string, vec2 position, color4 color);
    vec2 monospace_size(string::view string) const;

    void draw_line(vec2 start, vec2 end, color4 start_color, color4 end_color);
    void draw_line(float width, vec2 start, vec2 end, color4 core_color, color4 edge_color) { draw_line(width, start, end, core_color, core_color, edge_color, edge_color); }
    void draw_line(float width, vec2 start, vec2 end, color4 start_color, color4 end_color, color4 start_edge_color, color4 end_edge_color);
    void draw_box(vec2 size, vec2 position, color4 color);
    void draw_triangles(vec2 const* position, color4 const* color, int const* indices, std::size_t num_indices);
    void draw_particles(time_value time, render::particle const* particles, std::size_t num_particles);
    void draw_model(render::model const* model, mat3 transform, color4 color);
    void draw_starfield();

    void set_view(render::view const& view);

private:

    // More font stuff (r_font.cpp)

    std::unique_ptr<render::font> _default_font;
    std::unique_ptr<render::font> _monospace_font;

    std::vector<std::unique_ptr<render::font>> _fonts;

    std::vector<std::unique_ptr<render::image>> _images;

    // Internal stuff

    config::integer _framebuffer_width;
    config::integer _framebuffer_height;
    config::scalar _framebuffer_scale;
    config::integer _framebuffer_samples;

    GLuint _fbo;
    GLuint _rbo[2];
    vec2i _framebuffer_size;

    render::window* _window;

    void create_default_font();
    void set_default_state();
    void create_framebuffer(vec2i size, int samples);
    void destroy_framebuffer();

    render::view _view;

    config::boolean _draw_tris;

    std::vector<vec2> _starfield_points;
    std::vector<color3> _starfield_colors;

    float _costbl[360];
    float _sintbl[360];

private:

    // additional opengl bindings
    typedef void (APIENTRY* PFNGLBINDRENDERBUFFER)(GLenum target, GLuint renderbuffer);
    typedef void (APIENTRY* PFNGLDELETERENDERBUFFERS)(GLsizei n, GLuint const* renderbuffers);
    typedef void (APIENTRY* PFNGLGENRENDERBUFFERS)(GLsizei n, GLuint* renderbuffers);
    typedef void (APIENTRY* PFNGLRENDERBUFFERSTORAGE)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    typedef void (APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
    typedef void (APIENTRY* PFNGLBINDFRAMEBUFFER)(GLenum target, GLuint framebuffer);
    typedef void (APIENTRY* PFNGLDELETEFRAMEBUFFERS)(GLsizei n, GLuint const* framebuffers);
    typedef void (APIENTRY* PFNGLGENFRAMEBUFFERS)(GLsizei n, GLuint* framebuffers);
    typedef void (APIENTRY* PFNGLFRAMEBUFFERRENDERBUFFER)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    typedef void (APIENTRY* PFNGLBLITFRAMEBUFFER)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
    typedef void (APIENTRY* PFNGLBLENDCOLOR)(GLfloat red, GLfloat greed, GLfloat blue, GLfloat alpha);

    PFNGLBINDRENDERBUFFER glBindRenderbuffer = NULL;
    PFNGLDELETERENDERBUFFERS glDeleteRenderbuffers = NULL;
    PFNGLGENRENDERBUFFERS glGenRenderbuffers = NULL;
    PFNGLRENDERBUFFERSTORAGE glRenderbufferStorage = NULL;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLE glRenderbufferStorageMultisample = NULL;
    PFNGLBINDFRAMEBUFFER glBindFramebuffer = NULL;
    PFNGLDELETEFRAMEBUFFERS glDeleteFramebuffers = NULL;
    PFNGLGENFRAMEBUFFERS glGenFramebuffers = NULL;
    PFNGLFRAMEBUFFERRENDERBUFFER glFramebufferRenderbuffer = NULL;
    PFNGLBLITFRAMEBUFFER glBlitFramebuffer = NULL;
    PFNGLBLENDCOLOR glBlendColor = NULL;
};

} // namespace render
