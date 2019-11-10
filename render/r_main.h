// r_main.h
//

#pragma once

#include "cm_config.h"
#include "cm_string.h"
#include "cm_time.h"

#ifndef _WINDOWS_
#define WINAPI __stdcall
#define APIENTRY WINAPI
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
    void draw_arc(vec2 center, float radius, float width, float min_angle, float max_angle, color4 color);
    void draw_box(vec2 size, vec2 position, color4 color);
    void draw_triangles(vec2 const* position, color4 const* color, int const* indices, std::size_t num_indices);
    void draw_particles(time_value time, render::particle const* particles, std::size_t num_particles);
    void draw_model(render::model const* model, mat3 transform, color4 color);
    void draw_starfield(vec2 streak_vector = vec2_zero);

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

//------------------------------------------------------------------------------
class tree
{
public:
    using heightfield = float(*)(vec2);
    void build(bounds aabb, float resolution, heightfield fn) {
        _nodes.resize(4);
        _aabb = aabb;
        _nodes[0] = build_r(_aabb, resolution, fn);
    }

    //! Returns true if the height in the entire region of the given aabb is <= 0
    bool is_below(bounds aabb) const {
        return overlap_r(0, _aabb, aabb, BELOW);
    }

    //! Returns true if the height in the entire region of the given aabb is >= 0
    bool is_above(bounds aabb) const {
        return overlap_r(0, _aabb, aabb, ABOVE);
    }

    void draw(system* r, bounds view_aabb, float resolution = 0.f) const {
        draw_r(r, 0, _aabb, view_aabb,
            resolution ? std::ilogb(_aabb.size().length() / resolution) : INT_MAX);
    }

private:
    enum {
        BELOW = (1 << 0),
        ABOVE = (1 << 1),
        MIXED = BELOW | ABOVE,
    };

    struct node {
        uint32_t mask : 2;
        uint32_t child_index : 30;
    };

    bounds _aabb;
    std::vector<node> _nodes;

private:
    // 2 3
    // 0 1
    void split(bounds aabb, bounds (&out)[4]) const {
        vec2 min = aabb[0], max = aabb[1], mid = aabb.center();
        out[0] = bounds{{min.x, min.y}, {mid.x, mid.y}};
        out[1] = bounds{{mid.x, min.y}, {max.x, mid.y}};
        out[2] = bounds{{min.x, mid.y}, {mid.x, max.y}};
        out[3] = bounds{{mid.x, mid.y}, {max.x, max.y}};
    }

    node build_r(bounds aabb, float resolution, heightfield fn) {
        if (aabb.maxs().x - aabb.mins().x < resolution) {
            return fn(aabb.center()) < 0.f ? node{BELOW} : node{ABOVE};
        }

        uint32_t idx = narrow_cast<uint32_t>(_nodes.size());
        assert(idx < (1u << 30));
        _nodes.resize(_nodes.size() + 4);

        bounds children[4]; split(aabb, children);

        _nodes[idx + 0] = build_r(children[0], resolution, fn);
        _nodes[idx + 1] = build_r(children[1], resolution, fn);
        _nodes[idx + 2] = build_r(children[2], resolution, fn);
        _nodes[idx + 3] = build_r(children[3], resolution, fn);

        uint32_t mask = _nodes[idx + 0].mask
                      | _nodes[idx + 1].mask
                      | _nodes[idx + 2].mask
                      | _nodes[idx + 3].mask;

        if (mask != MIXED) {
            assert(_nodes.size() == idx + 4);
            _nodes.resize(idx);
            return node{mask};
        } else {
            return node{mask, idx};
        }
    }

    bool overlap_r(uint32_t node_index, bounds node_aabb, bounds aabb, uint32_t mask) const {
        uint32_t child_index = _nodes[node_index].child_index;

        if (!(_nodes[node_index].mask & mask)) {
            return false;
        } else if (!node_aabb.intersects(aabb)) {
            return false;
        } else if (!child_index) {
            return true;
        }

        bounds children[4]; split(node_aabb, children);
        return overlap_r(child_index + 0, children[0], aabb, mask)
            || overlap_r(child_index + 1, children[1], aabb, mask)
            || overlap_r(child_index + 2, children[2], aabb, mask)
            || overlap_r(child_index + 3, children[3], aabb, mask);
    }

    void draw_r(system* r, uint32_t node_index, bounds node_aabb, bounds view_aabb, int max_depth) const {
        if (!node_aabb.intersects(view_aabb)) {
            return;
        }
        uint32_t child_index = _nodes[node_index].child_index;
        if (!child_index || !max_depth) {
            r->draw_box(node_aabb.size(), node_aabb.center(),
                _nodes[node_index].mask == MIXED ? color4(0,1,1,.2f) :
                _nodes[node_index].mask == ABOVE ? color4(0,1,0,.2f) :
                _nodes[node_index].mask == BELOW ? color4(0,0,1,.2f) : color4(1,1,1,1));
        } else {
            bounds children[4]; split(node_aabb, children);
            draw_r(r, child_index + 0, children[0], view_aabb, max_depth - 1);
            draw_r(r, child_index + 1, children[1], view_aabb, max_depth - 1);
            draw_r(r, child_index + 2, children[2], view_aabb, max_depth - 1);
            draw_r(r, child_index + 3, children[3], view_aabb, max_depth - 1);
        }
    }
};

} // namespace render
