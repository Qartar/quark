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
    using heightfield_fn = std::function<float(vec2)>;

    tree()
        : _aabb{}
    {}

    //! Build tree over the given region using the heightfield function
    //! to classify leaves as either above or below 0. Leaf nodes will be
    //! no smaller than the given resolution along the x-axis.
    void build(bounds region, float resolution, heightfield_fn fn) {
        _aabb = region;
        // Child nodes are allocated contiguously, do the same for the
        // root node for consistency, this means nodes 1-3 are unused.
        _nodes.resize(4);
        _nodes[0] = build_r(_aabb, resolution, fn);
    }

    //! Returns true if the height in the entire region of the given aabb is < 0
    bool is_below(bounds aabb) const {
        return overlap_r(0, _aabb, aabb, BELOW);
    }

    //! Returns true if the height in the entire region of the given aabb is >= 0
    bool is_above(bounds aabb) const {
        return overlap_r(0, _aabb, aabb, ABOVE);
    }

    //! Returns the fraction along <start, end> to the first point on the line with height >= 0, or 1.f if no such point exists
    float trace_below(vec2 start, vec2 end) const {
        return trace(start, end, ABOVE);
    }

    //! Returns the fraction along <start, end> to the first point on the line with height < 0, or 1.f if no such point exists
    float trace_above(vec2 start, vec2 end) const {
        return trace(start, end, BELOW);
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
        uint32_t child_index : 30; //!< Index of first of four contiguous child nodes or 0 if this is a leaf
    };

    bounds _aabb; //!< Axis-aligned bounding box for the tree
    std::vector<node> _nodes;

private:
    //! Split the given bounds at its midpoint into four equally sized bounds.
    //!     2 3
    //!     0 1
    std::array<bounds, 4> split(bounds in) const {
        vec2 min = in[0], max = in[1], mid = in.center();
        return {
            bounds{{min.x, min.y}, {mid.x, mid.y}},
            bounds{{mid.x, min.y}, {max.x, mid.y}},
            bounds{{min.x, mid.y}, {mid.x, max.y}},
            bounds{{mid.x, mid.y}, {max.x, max.y}},
        };
    }

    node build_r(bounds aabb, float resolution, heightfield_fn fn) {
        // If at maximum resolution create leaf using the mask at the midpoint
        if (aabb.size().x < resolution) {
            return fn(aabb.center()) < 0.f ? node{BELOW} : node{ABOVE};
        }

        // Reserve space for contiguous child nodes
        uint32_t idx = narrow_cast<uint32_t>(_nodes.size());
        assert(idx < (1u << 30));
        _nodes.resize(_nodes.size() + 4);

        // Calculate bounds for each child node and recurse
        std::array<bounds, 4> children = split(aabb);

        _nodes[idx + 0] = build_r(children[0], resolution, fn);
        _nodes[idx + 1] = build_r(children[1], resolution, fn);
        _nodes[idx + 2] = build_r(children[2], resolution, fn);
        _nodes[idx + 3] = build_r(children[3], resolution, fn);

        // Node mask is the combination of all child masks
        uint32_t mask = _nodes[idx + 0].mask
                      | _nodes[idx + 1].mask
                      | _nodes[idx + 2].mask
                      | _nodes[idx + 3].mask;

        // If all children have the same mask then collapse the node
        if (mask != MIXED) {
            // Assert that all children have also been collapsed
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

        // Calculate bounds for each child node and recurse
        std::array<bounds, 4> children = split(node_aabb);
        return overlap_r(child_index + 0, children[0], aabb, mask)
            || overlap_r(child_index + 1, children[1], aabb, mask)
            || overlap_r(child_index + 2, children[2], aabb, mask)
            || overlap_r(child_index + 3, children[3], aabb, mask);
    }

    float trace(vec2 start, vec2 end, uint32_t mask) const {
        uint32_t trace_bits = 0;
        vec2 inverse_dir = {1.f / (end.x - start.x),
                            1.f / (end.y - start.y)};
        // Calculate intersection fractions with the root AABB
        vec2 tmin = (_aabb[0] - start) * inverse_dir;
        vec2 tmax = (_aabb[1] - start) * inverse_dir;
        // If going in -x direction swap min/max and traverse order
        if (tmin.x > tmax.x) {
            std::swap(tmin.x, tmax.x);
            trace_bits ^= 1;
        }
        // If going in -y direction swap min/max and traverse order
        if (tmin.y > tmax.y) {
            std::swap(tmin.y, tmax.y);
            trace_bits ^= 2;
        }
        return trace_r(0, trace_bits, {tmin, tmax}, mask);
    }

    float trace_r(uint32_t node_index, uint32_t trace_bits, bounds trace_aabb, uint32_t mask) const {
        uint32_t child_index = _nodes[node_index].child_index;

        float t0 = max(trace_aabb[0].x, trace_aabb[0].y);
        float t1 = min(trace_aabb[1].x, trace_aabb[1].y);

        if (!(_nodes[node_index].mask & mask)) {
            return 1.f;
        } else if (t0 >= t1 || t0 >= 1.f || t1 < 0.f) {
            return 1.f;
        } else if (!child_index) {
            return max(0.f, t0);
        }

        // Intersection fraction with the splitting planes is just the
        // midpoint of the intersection fractions of the full AABB.
        std::array<bounds, 4> children = split(trace_aabb);
        for (int ii = 0; ii < 4; ++ii) {
            // Trace bits control the order of traversal
            float t = trace_r(child_index + (ii ^ trace_bits),
                              trace_bits,
                              children[ii],
                              mask);
            // Traversal order guarantees that the first intersection
            // is the nearest intersection
            if (t < 1.f) {
                return t;
            }
        }

        return 1.f;
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
            std::array<bounds, 4> children = split(node_aabb);
            draw_r(r, child_index + 0, children[0], view_aabb, max_depth - 1);
            draw_r(r, child_index + 1, children[1], view_aabb, max_depth - 1);
            draw_r(r, child_index + 2, children[2], view_aabb, max_depth - 1);
            draw_r(r, child_index + 3, children[3], view_aabb, max_depth - 1);
        }
    }
};

//------------------------------------------------------------------------------
class minmax_tree
{
public:
    using heightfield_fn = std::function<float(vec2)>;

    minmax_tree()
        : _aabb{}
    {}

    //! Build min-max tree over the given region using the heightfield function.
    //! Leaf nodes will be no smaller than the given resolution along the x-axis.
    void build(bounds region, float resolution, heightfield_fn fn) {
        _aabb = region;
        // Child nodes are allocated contiguously, do the same for the
        // root node for consistency, this means nodes 1-3 are unused.
        _nodes.resize(4);
        _nodes[0] = build_r(_aabb, resolution, fn);
    }

    //! Returns the fraction along <start, end> to the first point on the line
    //! which intersects the heightfield.
    float trace(vec3 start, vec3 end) const {
        uint32_t trace_bits = 0;
        vec2 inverse_dir = {1.f / (end.x - start.x),
                            1.f / (end.y - start.y)};
        // Calculate intersection fractions with the root AABB
        vec2 tmin = (_aabb[0] - start.to_vec2()) * inverse_dir;
        vec2 tmax = (_aabb[1] - start.to_vec2()) * inverse_dir;
        // If going in -x direction swap min/max and traverse order
        if (tmin.x > tmax.x) {
            std::swap(tmin.x, tmax.x);
            trace_bits ^= 1;
        }
        // If going in -y direction swap min/max and traverse order
        if (tmin.y > tmax.y) {
            std::swap(tmin.y, tmax.y);
            trace_bits ^= 2;
        }
        return trace_r(0, trace_bits, {tmin, tmax}, start.z, end.z - start.z);
    }

    void draw(system* r, bounds view_aabb, float resolution = 0.f) const {
        draw_r(r, 0, _aabb, view_aabb,
            resolution ? std::ilogb(_aabb.size().length() / resolution) : INT_MAX);
    }

private:
    struct node {
        float min;
        float max;
        uint32_t child_index; //!< Index of first of four contiguous child nodes or 0 if this is a leaf
    };

    bounds _aabb; //!< Axis-aligned bounding box for the tree
    std::vector<node> _nodes;

private:
    //! Split the given bounds at its midpoint into four equally sized bounds.
    //!     2 3
    //!     0 1
    std::array<bounds, 4> split(bounds in) const {
        vec2 min = in[0], max = in[1], mid = in.center();
        return {
            bounds{{min.x, min.y}, {mid.x, mid.y}},
            bounds{{mid.x, min.y}, {max.x, mid.y}},
            bounds{{min.x, mid.y}, {mid.x, max.y}},
            bounds{{mid.x, mid.y}, {max.x, max.y}},
        };
    }

    node build_r(bounds aabb, float resolution, heightfield_fn fn) {
        // If at maximum resolution create leaf using height at each corner
        if (aabb.size().x < resolution) {
            float const h[4] = {
                fn(vec2(aabb[0][0], aabb[0][1])),
                fn(vec2(aabb[1][0], aabb[0][1])),
                fn(vec2(aabb[1][0], aabb[1][1])),
                fn(vec2(aabb[0][0], aabb[1][1])),
            };
            float hmin = h[0], hmax = h[0];
            for (int ii = 1; ii < countof(h); ++ii) {
                hmin = min(h[ii], hmin);
                hmax = max(h[ii], hmax);
            }
            return node{hmin, hmax};
        }

        // Reserve space for contiguous child nodes
        uint32_t idx = narrow_cast<uint32_t>(_nodes.size());
        assert(idx < (1u << 30));
        _nodes.resize(_nodes.size() + 4);

        // Calculate bounds for each child node and recurse
        std::array<bounds, 4> children = split(aabb);

        _nodes[idx + 0] = build_r(children[0], resolution, fn);
        _nodes[idx + 1] = build_r(children[1], resolution, fn);
        _nodes[idx + 2] = build_r(children[2], resolution, fn);
        _nodes[idx + 3] = build_r(children[3], resolution, fn);

        float min = std::min({_nodes[idx + 0].min,
                              _nodes[idx + 1].min,
                              _nodes[idx + 2].min,
                              _nodes[idx + 3].min});

        float max = std::max({_nodes[idx + 0].max,
                              _nodes[idx + 1].max,
                              _nodes[idx + 2].max,
                              _nodes[idx + 3].max});

        return node{min, max, idx};
    }

    float trace_r(uint32_t node_index, uint32_t trace_bits, bounds trace_aabb, float h, float dhdt) const {
        uint32_t child_index = _nodes[node_index].child_index;

        float t0 = max(trace_aabb[0].x, trace_aabb[0].y);
        float t1 = min(trace_aabb[1].x, trace_aabb[1].y);

        float h0 = h + t0 * dhdt;
        float h1 = h + t1 * dhdt;

        if (t0 >= t1 || t0 >= 1.f || t1 < 0.f) {
            return 1.f;
        } else if (min(h0, h1) > _nodes[node_index].max || max(h0, h1) < _nodes[node_index].min) {
            return 1.f;
        } else if (!child_index) {
            // TODO: Return the intersection fraction against the implicit surface
            // Find intersection fraction on the z-plane
            float th = dhdt > 0.f ? (_nodes[node_index].min - h) / dhdt
                     : dhdt < 0.f ? (_nodes[node_index].max - h) / dhdt
                     : t0;
            // Return intersection fraction against the leaf's AABB
            return max(t0, th);
        }

        // Intersection fraction with the splitting planes is just the
        // midpoint of the intersection fractions of the full AABB.
        std::array<bounds, 4> children = split(trace_aabb);
        for (int ii = 0; ii < 4; ++ii) {
            // Trace bits control the order of traversal
            float t = trace_r(child_index + (ii ^ trace_bits),
                              trace_bits,
                              children[ii],
                              h,
                              dhdt);
            // Traversal order guarantees that the first intersection
            // is the nearest intersection
            if (t < 1.f) {
                return t;
            }
        }

        return 1.f;
    }

    void draw_r(system* r, uint32_t node_index, bounds node_aabb, bounds view_aabb, int max_depth) const {
        if (!node_aabb.intersects(view_aabb)) {
            return;
        }
        uint32_t child_index = _nodes[node_index].child_index;
        if (!child_index || !max_depth) {
            r->draw_box(node_aabb.size(), node_aabb.center(),
                _nodes[node_index].min < 0.f && _nodes[node_index].max > 0.f ? color4(0,1,1,.2f) :
                _nodes[node_index].min > 0.f ? color4(0,1,0,.2f) :
                _nodes[node_index].max < 0.f ? color4(0,0,1,.2f) : color4(1,1,1,1));
        } else if (_nodes[node_index].min > 0.f) {
            r->draw_box(node_aabb.size(), node_aabb.center(), color4(0,1,0,.2f));
        } else if (_nodes[node_index].max < 0.f) {
            r->draw_box(node_aabb.size(), node_aabb.center(), color4(0,0,1,.2f));
        } else {
            std::array<bounds, 4> children = split(node_aabb);
            draw_r(r, child_index + 0, children[0], view_aabb, max_depth - 1);
            draw_r(r, child_index + 1, children[1], view_aabb, max_depth - 1);
            draw_r(r, child_index + 2, children[2], view_aabb, max_depth - 1);
            draw_r(r, child_index + 3, children[3], view_aabb, max_depth - 1);
        }
    }
};

//------------------------------------------------------------------------------
class line_tree
{
public:
    using heightfield_fn = std::function<float(vec2)>;

    line_tree()
        : _aabb{}
    {}

    //! Build tree over the given region using the heightfield function
    //! to classify leaves as either above or below 0. Leaf nodes will be
    //! no smaller than the given resolution along the x-axis.
    void build(bounds region, float resolution, heightfield_fn fn) {
        _aabb = region;
        // Child nodes are allocated contiguously, do the same for the
        // root node for consistency, this means nodes 1-3 are unused.
        _nodes.resize(4);
        _nodes[0] = build_r(_aabb,
                            std::ilogb(_aabb.size().length() / resolution) - leaf_depth,
                            fn);
    }

    void draw(system* r, bounds view_aabb, color4 color, float resolution = 0.f) const;

private:
    constexpr static int leaf_depth = 8;
    constexpr static int leaf_size = 1 << leaf_depth;

    bounds _aabb;
    struct node {
        uint32_t child_index;
        uint32_t vertex_offset;
        uint32_t vertex_count;
    };
    std::vector<node> _nodes;
    std::vector<vec2> _vertices;

private:
    //! Split the given bounds at its midpoint into four equally sized bounds.
    //!     2 3
    //!     0 1
    std::array<bounds, 4> split(bounds in) const {
        vec2 min = in[0], max = in[1], mid = in.center();
        return {
            bounds{{min.x, min.y}, {mid.x, mid.y}},
            bounds{{mid.x, min.y}, {max.x, mid.y}},
            bounds{{min.x, mid.y}, {mid.x, max.y}},
            bounds{{mid.x, mid.y}, {max.x, max.y}},
        };
    }

    node build_r(bounds aabb, int max_depth, heightfield_fn fn) {
        node out = tessellate(aabb, fn);
        if (max_depth > 0) {
            out.child_index = narrow_cast<uint32_t>(_nodes.size());
            _nodes.resize(out.child_index + 4);

            // Calculate bounds for each child node and recurse
            std::array<bounds, 4> children = split(aabb);

            _nodes[out.child_index + 0] = build_r(children[0], max_depth - 1, fn);
            _nodes[out.child_index + 1] = build_r(children[1], max_depth - 1, fn);
            _nodes[out.child_index + 2] = build_r(children[2], max_depth - 1, fn);
            _nodes[out.child_index + 3] = build_r(children[3], max_depth - 1, fn);

            // If all children have no vertices then collapse the node
            if (!_nodes[out.child_index + 0].vertex_count
                && !_nodes[out.child_index + 1].vertex_count
                && !_nodes[out.child_index + 2].vertex_count
                && !_nodes[out.child_index + 3].vertex_count) {

                // Assert that all children have also been collapsed
                assert(_nodes.size() == out.child_index + 4);
                _nodes.resize(out.child_index);
                return node{};
            }
        }
        return out;
    }

    node tessellate(bounds aabb, heightfield_fn fn) {
        float dx = aabb.size().x / leaf_size;
        float dy = aabb.size().y / leaf_size;

        node out = {};
        out.vertex_offset = narrow_cast<uint32_t>(_vertices.size());

        auto emit_vertex = [this, &out](vec2 v) {
            _vertices.push_back(v);
            ++out.vertex_count;
        };

        for (int jj = 0; jj < leaf_size; ++jj) {
            float y0 = aabb[0][1] + dy * jj;

            for (int ii = 0; ii < leaf_size; ++ii) {
                float x0 = aabb[0][0] + dx * ii;

                float h00, h10;
                float h01, h11;

                h00 = fn(vec2(x0,      y0     ));
                h10 = fn(vec2(x0 + dx, y0     ));
                h01 = fn(vec2(x0,      y0 + dy));
                h11 = fn(vec2(x0 + dx, y0 + dy));

                float x2 = x0 - dx * h00 / (h10 - h00);
                float x3 = x0 - dx * h01 / (h11 - h01);
                float y2 = y0 - dy * h00 / (h01 - h00);
                float y3 = y0 - dy * h10 / (h11 - h10);

                int mask = ((h00 < 0.f) ? 1 : 0)
                         | ((h10 < 0.f) ? 2 : 0)
                         | ((h01 < 0.f) ? 4 : 0)
                         | ((h11 < 0.f) ? 8 : 0);

                switch (mask) {
                case 1:
                case 15 ^ 1:
                    emit_vertex({x0, y2});
                    emit_vertex({x2, y0});
                    break;

                case 2:
                case 15 ^ 2:
                    emit_vertex({x2, y0});
                    emit_vertex({x0 + dx, y3});
                    break;

                case 4:
                case 15 ^ 4:
                    emit_vertex({x0, y2});
                    emit_vertex({x3, y0 + dy});
                    break;

                case 8:
                case 15 ^ 8:
                    emit_vertex({x3, y0 + dy});
                    emit_vertex({x0 + dx, y3});
                    break;

                case 1 | 2:
                case 4 | 8:
                    emit_vertex({x0, y2});
                    emit_vertex({x0 + dx, y3});
                    break;

                case 1 | 4:
                case 2 | 8:
                    emit_vertex({x2, y0});
                    emit_vertex({x3, y0 + dx});
                    break;

                case 1 | 8:
                    emit_vertex({x2, y0});
                    emit_vertex({x0 + dx, y3});
                    emit_vertex({x0, y2});
                    emit_vertex({x3, y0 + dy});
                    break;

                case 2 | 4:
                    emit_vertex({x0, y2});
                    emit_vertex({x2, y0});
                    emit_vertex({x3, y0 + dy});
                    emit_vertex({x0 + dx, y3});
                    break;
                }
            }
        }

        return out;
    }

    void draw_r(system* r, uint32_t node_index, bounds node_aabb, bounds view_aabb, int max_depth) const {
        if (!node_aabb.intersects(view_aabb)) {
            return;
        }
        uint32_t child_index = _nodes[node_index].child_index;
        if (!child_index || max_depth <= 0) {
            draw_lines(node_index);
        } else {
            std::array<bounds, 4> children = split(node_aabb);
            draw_r(r, child_index + 0, children[0], view_aabb, max_depth - 1);
            draw_r(r, child_index + 1, children[1], view_aabb, max_depth - 1);
            draw_r(r, child_index + 2, children[2], view_aabb, max_depth - 1);
            draw_r(r, child_index + 3, children[3], view_aabb, max_depth - 1);
        }
    }

    void draw_lines(uint32_t node_index) const;
};

} // namespace render
