// g_ship_editor.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_keys.h"
#include "g_ship_editor.h"
#include "r_image.h"
#include "r_window.h"

#include "cm_filesystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

#define SHIP(L,B)               \
    vec2(-0.5f * L, 0.f),       \
    vec2(-0.5f * L, 0.5f * B),  \
    vec2(0.f, 0.5f * B),        \
    vec2(0.3f * L, 0.5f * B),   \
    vec2(0.5f * L, 0.f),


#define SHIP_CUBE(L,B)          \
    vec2(-0.5f * L, 0.f),       \
    vec2(-0.5f * L, 0.5f * B),  \
    vec2(-0.25f * L, 0.5f * B), \
    vec2(-0.1f * L, 0.5f * B),  \
    vec2(0.1f * L, 0.5f * B),   \
    vec2(0.25f * L, 0.5f * B),  \
    vec2(0.3f * L, 0.5f * B),   \
    vec2(0.5f * L, 0.f),

#define SHIP_CUBE2(L,B)          \
    vec2(-0.5f * L, 0.f),       \
    vec2(-0.5f * L, 0.5f * B),  \
    vec2(-0.25f * L, 0.5f * B), \
    vec2(0, 0.5f * B),          \
    vec2(0.25f * L, 0.5f * B),  \
    vec2(0.3f * L, 0.5f * B),   \
    vec2(0.5f * L, 0.f),

//------------------------------------------------------------------------------
ship_editor::ship_editor()
    : _view{}
    , _cursor{}
    , _snap_distance(1.f)
    , _snap_to_grid(true)
    , _snap_to_edge(true)
    , _draw_grid(true)
    , _draw_linearized(false)
    , _is_panning(false)
    , _is_panning_image(false)
    , _control(false)
    , _is_dragging(false)
    , _drag_index(0)
    , _image(nullptr)
    , _image_offset(vec2_zero)
    , _image_scale("image_scale", 1.f/15.175f, 0, "")
    , _mode(editor_mode::deck)
    , _turret_instance(0)
{
    _view.viewport.maxs() = application::singleton()->window()->size();
    _view.size = {64.f, 64.f * float(_view.viewport.maxs().y) / float(_view.viewport.maxs().x)};

    _image = application::singleton()->window()->renderer()->load_image(
        //"C:\\Users\\Carter\\OneDrive\\Pictures\\Trade Wars\\Constellation.bmp"
        //"D:\\Users\\Carter\\Pictures\\Yamato1945.bmp"
        //"D:\\Users\\Carter\\Pictures\\Kongo1944.bmp"
        //"D:\\Users\\Carter\\Pictures\\Fuso1944.bmp"
        //"D:\\Users\\Carter\\Pictures\\Iowa_classe_battleships_drawing.bmp"
        //"D:\\Users\\Carter\\Pictures\\KGV-as-built.bmp"
        //"D:\\Users\\Carter\\Pictures\\3607_Richelieu1941-09BattleofDakar_20231216154932.bmp"
        "D:\\Users\\Carter\\Pictures\\6474_Bismarck1941-05-271_20240302131526.bmp"
        //"D:\\Users\\Carter\\Pictures\\Littorio_class_battleship-drawing-2views.bmp"
    );

    _deck_vertices = { SHIP(219.61f, 33.1f) };
    _deck_segments = { quad, quad };

    //_deck_vertices = { SHIP_CUBE(219.61f, 28.04f) }; // Kongo
    //_deck_vertices = { SHIP_CUBE(210.f, 33.1f) }; // Fuso
    //_deck_vertices = { SHIP_CUBE(270.f, 33.f) }; // Iowa
    //_deck_vertices = { SHIP_CUBE(227.f, 31.5f) }; // KGV
    //_deck_segments = { cube, line, cube };
    //_deck_vertices = { SHIP_CUBE2(247.85f, 33.08f) }; // Richelieu (scale 0.1524)
    _deck_vertices = { SHIP_CUBE2(251.f, 36.f) }; // Bismarck (scale 0.1503)
    _deck_segments = { cube, cube };
    //_deck_vertices = { SHIP_CUBE(237.76f, 32.82f) }; // Littorio (scale 0.415)
    //_deck_segments = { cube, line, cube };
}

//------------------------------------------------------------------------------
ship_editor::~ship_editor()
{
}

//------------------------------------------------------------------------------
vec2 ship_editor::cursor_to_world() const
{
    //return _view.origin + _view.size * (_cursor / 
    //    vec2(_view.viewport.maxs() - _view.viewport.mins()) - vec2(.5f,.5f));
    return _view.origin + _view.size * _cursor;
}

//------------------------------------------------------------------------------
vec2 ship_editor::snap_vertex(vec2 pos) const
{
    vec2 out = pos;

    vec2 grid_snap = {
        std::floor(pos.x / _snap_distance + .5f) * _snap_distance,
        std::floor(pos.y / _snap_distance + .5f) * _snap_distance,
    };

    if (_snap_to_grid) {
        out = grid_snap;
    }

    return out;
}

//------------------------------------------------------------------------------
void ship_editor::draw(render::system* renderer, time_value /*time*/) const
{
    if (_image) {
        renderer->set_view(_view);
        vec2 image_size = vec2(vec2i(_image->width(), _image->height())) * _image_scale;
        renderer->draw_image(_image, _image_offset - .5f * image_size, image_size, color4(1,1,1,.5f));
    }

    renderer->set_view(_view);
    vec2 vmin = _view.origin - .5f * _view.size;
    vec2 vmax = _view.origin + .5f * _view.size;

    //
    // draw grid
    //

    {
        color4 c(1.f, 1.f, 1.f, .2f);
        renderer->draw_line(vec2(0, vmin.y), vec2(0, vmax.y), c, c);
        renderer->draw_line(vec2(vmin.x, 0), vec2(vmax.x, 0), c, c);
        if (_draw_grid) {
            vec2 mins = _snap_distance * vec2(std::round((vmin.x) / _snap_distance),
                                              std::round((vmin.y) / _snap_distance));
            vec2 maxs = _view.origin + .5f * _view.size;

            for (float x = mins.x; x < maxs.x; x += _snap_distance) {
                renderer->draw_line(vec2(x, vmin.y), vec2(x, vmax.y), c, c);
            }
            for (float y = mins.y; y < maxs.y; y += _snap_distance) {
                renderer->draw_line(vec2(vmin.x, y), vec2(vmax.x, y), c, c);
            }
        }
    }

    //
    // draw deck outline
    //

    if (_draw_linearized) {
        draw_transformed(renderer, mat3_identity, _deck_linearized);
    } else {
        draw_transformed(renderer, mat3_identity, _deck_vertices, _deck_segments);
    }

    //
    // draw turrets
    //

    for (std::size_t ii = 0; ii < _turret_instances.size(); ++ii) {
        turret_instance const& i = _turret_instances[ii];
        if (_draw_linearized) {
            draw_transformed(renderer, i.transform, _turrets[i.index].linearized);
        } else {
            draw_transformed(renderer, i.transform, _turrets[i.index].vertices, _turrets[i.index].segments);
            vec2 center = vec2_zero * i.transform;
            float offset = render_vertex_size() * 2.f;
            renderer->draw_line(center - vec2(0,offset), center + vec2(0,offset), color4(0,1,1,1), color4(0,1,1,1));
            renderer->draw_line(center - vec2(offset,0), center + vec2(offset,0), color4(0,1,1,1), color4(0,1,1,1));

            if (_mode == editor_mode::turret && ii == _turret_instance) {
                renderer->draw_arc(center, _turrets[i.index].radius, 0, 0, 2.f * math::pi, color4(0,1,1,1));

                vec2 orientation = vec2(_turrets[i.index].radius + offset, 0) * i.transform;
                renderer->draw_line(center, orientation, color4(0,1,1,1), color4(0,1,1,1));
                renderer->draw_arc(orientation, offset, 0.f, 0, 2.f * math::pi, color4(0,1,1,1));
            }
        }
    }

    //
    // draw vertex highlight and closest point
    //

    if (_mode == editor_mode::deck) {
        draw_highlight(renderer, mat3_identity, _deck_vertices, _deck_segments);
    } else if (_mode == editor_mode::turret) {
        if (_turret_instance < _turret_instances.size()) {
            std::size_t index = _turret_instances[_turret_instance].index;
            draw_highlight(renderer, _turret_instances[_turret_instance].transform, _turrets[index].vertices, _turrets[index].segments);
        }
    }

    vec2 world_pos = snap_vertex(cursor_to_world());

    {
        string::view s = va("(%g, %g)", world_pos.x, world_pos.y);
        vec2 size = renderer->string_size(s);
        renderer->draw_string(s, _view.origin + _view.size * .5f - size, color4(1,1,1,.5f));
    }
    {
        vec2 size = renderer->monospace_size("foo");
        renderer->draw_monospace("(s) snap to grid", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*1.f), color4(1,1,1, _snap_to_grid ? .6f : .3f));
        renderer->draw_monospace("( ) snap to edge", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*2.f), color4(1,1,1, _snap_to_edge ? .6f : .3f));

        renderer->draw_monospace("(d) deck mode", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*4.f), color4(1,1,1, _mode == editor_mode::deck ? .6f : .3f));
        renderer->draw_monospace("(t) turret mode", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*5.f), color4(1,1,1, _mode == editor_mode::turret ? .6f : .3f));
    }
}

//------------------------------------------------------------------------------
void ship_editor::draw_bezier_quad(render::system* renderer, vec2 a, vec2 b, vec2 c, color4 color) const
{
    vec2 v0 = a;
    for (int ii = 1; ii < 32; ++ii) {
        float t = ii / 32.f;
        vec2 v1 = (1-t)*(1-t)*a + 2*(1-t)*t*b + t*t*c;
        renderer->draw_line(v0, v1, color, color);
        v0 = v1;
    }
    renderer->draw_line(v0, c, color, color);
}

//------------------------------------------------------------------------------
void ship_editor::draw_bezier_cube(render::system* renderer, vec2 a, vec2 b, vec2 c, vec2 d, color4 color) const
{
    vec2 v0 = a;
    for (int ii = 1; ii < 32; ++ii) {
        float t = ii / 32.f;
        float t2 = t * t;
        float s = 1.f - t;
        float s2 = s * s;
        vec2 v1 = s2 * s * a + 3.f * s2 * t * b + 3.f * s * t2 * c + t * t2 * d;
        renderer->draw_line(v0, v1, color, color);
        v0 = v1;
    }
    renderer->draw_line(v0, d, color, color);
}

//------------------------------------------------------------------------------
void ship_editor::draw_transformed(render::system* renderer, mat3 transform, std::vector<vec2> const& linearized) const
{
    vec2 vertex_size = vec2(render_vertex_size());

    if (!linearized.size()) {
        return;
    }

    vec2 v0 = linearized[0] * transform;
    for (size_t ii = 0; ii + 1 < linearized.size(); ++ii) {
        vec2 v1 = linearized[ii + 1] * transform;
        renderer->draw_line(v0, v1, color4(1,1,1,1), color4(1,1,1,1));
        renderer->draw_box(vertex_size, v0, color4(1,0,0,1));
        v0 = v1;
    }
    vec2 v1 = linearized[0] * transform;
    renderer->draw_line(v0, v1, color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_box(vertex_size, v0, color4(1,0,0,1));
}

//------------------------------------------------------------------------------
void ship_editor::draw_transformed(render::system* renderer, mat3 transform, std::vector<vec2> const& vertices, std::vector<segment_type> const& segments) const
{
    vec2 vertex_size = vec2(render_vertex_size());

    for (size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (segments[ii] == line) {
            vec2 a = vertices[jj + 0] * transform;
            vec2 b = vertices[jj + 1] * transform;
            renderer->draw_line(a, b, color4(1,1,1,1), color4(1,1,1,1));
            renderer->draw_box(vertex_size, a, color4(1,0,0,1));
            renderer->draw_box(vertex_size, b, color4(1,0,0,1));
            jj += 1;
        } else if (segments[ii] == quad) {
            vec2 a = vertices[jj + 0] * transform;
            vec2 b = vertices[jj + 1] * transform;
            vec2 c = vertices[jj + 2] * transform;
            draw_bezier_quad(renderer, a, b, c, color4(1,1,1,1));
            renderer->draw_box(vertex_size, a, color4(1,0,0,1));
            renderer->draw_box(vertex_size, b, color4(1,1,0,1));
            renderer->draw_box(vertex_size, c, color4(1,0,0,1));
            jj += 2;
        } else if (segments[ii] == cube) {
            vec2 a = vertices[jj + 0] * transform;
            vec2 b = vertices[jj + 1] * transform;
            vec2 c = vertices[jj + 2] * transform;
            vec2 d = vertices[jj + 3] * transform;
            draw_bezier_cube(renderer, a, b, c, d, color4(1,1,1,1));
            renderer->draw_box(vertex_size, a, color4(1,0,0,1));
            renderer->draw_box(vertex_size, b, color4(1,1,0,1));
            renderer->draw_box(vertex_size, c, color4(1,1,0,1));
            renderer->draw_box(vertex_size, d, color4(1,0,0,1));
            jj += 3;
        }
    }
}

//------------------------------------------------------------------------------
void ship_editor::draw_highlight(render::system* renderer, mat3 transform, std::vector<vec2> const& vertices, std::vector<segment_type> const& segments) const
{
    vec2 vertex_size = vec2(render_vertex_size());

    vec2 vmin = _view.origin - .5f * _view.size;
    vec2 vmax = _view.origin + .5f * _view.size;

    std::size_t best_idx = SIZE_MAX;
    float best_dsqr = 1.f;
    vec2 p = cursor_to_world() * transform.inverse_transform();
    for (std::size_t ii = 0; ii < vertices.size(); ++ii) {
        float dsqr = length_sqr(vertices[ii] - p);
        if (dsqr < best_dsqr) {
            best_idx = ii;
            best_dsqr = dsqr;
        }
    }
    if (best_idx != SIZE_MAX) {
        vec2 v = vertices[best_idx] * transform;
        renderer->draw_box(vertex_size, v, color4(0,1,0,1));
        renderer->draw_line(vec2(v.x, vmin.y), vec2(v.x, vmax.y), color4(0,1,0,.2f), color4(0,1,0,.2f));
        renderer->draw_line(vec2(vmin.x, v.y), vec2(vmax.x, v.y), color4(0,1,0,.2f), color4(0,1,0,.2f));
    } else {
        vec2 v = closest_point(vertices, segments, p) * transform;
        renderer->draw_box(vertex_size, v, color4(0,1,1,1));
        renderer->draw_line(vec2(v.x, vmin.y), vec2(v.x, vmax.y), color4(0,1,1,.2f), color4(0,1,1,.2f));
        renderer->draw_line(vec2(vmin.x, v.y), vec2(vmax.x, v.y), color4(0,1,1,.2f), color4(0,1,1,.2f));
    }
}

//------------------------------------------------------------------------------
void ship_editor::draw_turret_highlight(render::system* renderer, turret_instance const& instance) const
{
#if 1
    (void)renderer;
    (void)instance;
#else
    vec2 vertex_size = vec2(render_vertex_size());

    vec2 vmin = _view.origin - .5f * _view.size;
    vec2 vmax = _view.origin + .5f * _view.size;

    std::size_t best_idx = SIZE_MAX;
    float best_dsqr = 1.f;
    vec2 p = cursor_to_world() * transform.inverse_transform();
    for (std::size_t ii = 0; ii < vertices.size(); ++ii) {
        float dsqr = length_sqr(vertices[ii] - p);
        if (dsqr < best_dsqr) {
            best_idx = ii;
            best_dsqr = dsqr;
        }
    }
    if (best_idx != SIZE_MAX) {
        vec2 v = vertices[best_idx] * transform;
        renderer->draw_box(vertex_size, v, color4(0,1,0,1));
        renderer->draw_line(vec2(v.x, vmin.y), vec2(v.x, vmax.y), color4(0,1,0,.2f), color4(0,1,0,.2f));
        renderer->draw_line(vec2(vmin.x, v.y), vec2(vmax.x, v.y), color4(0,1,0,.2f), color4(0,1,0,.2f));
    } else {
        vec2 v = closest_point(vertices, segments, p) * transform;
        renderer->draw_box(vertex_size, v, color4(0,1,1,1));
        renderer->draw_line(vec2(v.x, vmin.y), vec2(v.x, vmax.y), color4(0,1,1,.2f), color4(0,1,1,.2f));
        renderer->draw_line(vec2(vmin.x, v.y), vec2(vmax.x, v.y), color4(0,1,1,.2f), color4(0,1,1,.2f));
    }
#endif
}

//------------------------------------------------------------------------------
float segment_closest_point(vec2 a, vec2 b, vec2 p)
{
    vec2 v = b - a;
    float num = dot(p - a, v);
    float den = dot(v, v);
    if (num >= den) {
        return 1.f;
    } else if (num <= 0.f) {
        return 0.f;
    } else {
        return num / den;
    }
}

//------------------------------------------------------------------------------
// https://www.shadertoy.com/view/MlKcDD
// Copyright © 2018 Inigo Quilez
float quad_closest_point(vec2 A, vec2 B, vec2 C, vec2 pos)
{
    vec2 a = B - A;
    vec2 b = A - 2.f * B + C;
    vec2 c = a * 2.f;
    vec2 d = A - pos;

    float kk = 1.f / dot(b, b);
    float kx = kk * dot(a, b);
    float ky = kk * (2.f * dot(a, a) + dot(d, b)) / 3.f;
    float kz = kk * dot(d, a);

    float res = 0.f;
    float sgn = 0.f;
    float t = 0.f;

    float p = ky - kx * kx;
    float p3 = p * p * p;
    float q = kx * (2.f * kx * kx - 3.f * ky) + kz;
    float h = q * q + 4.f * p3;

    if (h >= 0.f) {
        h = std::sqrt(h);
        vec2 x = (vec2(h, -h) - vec2(q)) / 2.f;
        vec2 uv = vec2(std::cbrt(x.x), std::cbrt(x.y));
        t = clamp(uv.x + uv.y - kx, 0.f, 1.f);
        vec2 qx = d + (c + b * t) * t;
        res = dot(qx, qx);
        sgn = cross(c + 2.f * b * t, qx);
    } else {
        float z = std::sqrt(-p);
        float v = std::acos(q / (p * z * 2.f)) / 3.f;
        float m = std::cos(v);
        float n = std::sin(v) * 1.732050808f; // sqrt(3)

        float tx = clamp((m + m) * z - kx, 0.f, 1.f);
        vec2 qx = d + (c + b * tx) * tx;
        float dx = dot(qx, qx);
        float sx = cross(c + 2.f * b * tx, qx);

        float ty = clamp((-n - m) * z - kx, 0.f, 1.f);
        vec2 qy = d + (c + b * ty) * ty;
        float dy = dot(qy, qy);
        float sy = cross(c + 2.f * b * ty, qy);

        // the third root cannot be the closest
        //float tz = clamp((n - m) * z - kx, 0.f, 1.f);
        //  ...

        res = (dx < dy) ? dx : dy;
        sgn = (dx < dy) ? sx : sy;
        t = (dx < dy) ? tx : ty;
    }

    return t;
    //return (1-t)*(1-t)*A + 2*(1-t)*t*B + t*t*C;
/*
    if (t == 0.f) {
        float den = dot(B - A, B - A);
        float det = cross(B - A, pos - A);
        return {std::copysign(res, -sgn), det * det / den};
    } else if (t == 1.f) {
        float den = dot(C - B, C - B);
        float det = cross(C - B, pos - C);
        return {std::copysign(res, -sgn), det * det / den};
    } else {
        return {std::copysign(res, -sgn), res};
    }
*/
}

//------------------------------------------------------------------------------
float cube_closest_point(vec2 A, vec2 B, vec2 C, vec2 D, vec2 pos)
{
    (void)A;
    (void)B;
    (void)C;
    (void)D;
    (void)pos;
    return 0.5f;
}

//------------------------------------------------------------------------------
std::size_t ship_editor::closest_vertex(std::vector<vec2> const& vertices, vec2 v) const
{
    std::size_t best_idx = SIZE_MAX;
    float best_dsqr = FLT_MAX;
    for (std::size_t ii = 0; ii < vertices.size(); ++ii) {
        float dsqr = length_sqr(vertices[ii] - v);
        if (dsqr < best_dsqr) {
            best_idx = ii;
            best_dsqr = dsqr;
        }
    }
    return best_idx;
}

//------------------------------------------------------------------------------
std::size_t ship_editor::closest_segment(std::vector<vec2> const& vertices, std::vector<segment_type> const& segments, vec2 v) const
{
    std::size_t best_idx = SIZE_MAX;
    float best_dsqr = FLT_MAX;
    for (std::size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (segments[ii] == line) {
            float t = segment_closest_point(vertices[jj + 0], vertices[jj + 1], v);
            vec2 p = vertices[jj + 0] + (vertices[jj + 1] - vertices[jj + 0]) * t;
            float dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_dsqr = dsqr;
            }
            jj += 1;
        } else if (segments[ii] == quad) {
            float t = quad_closest_point(vertices[jj + 0], vertices[jj + 1], vertices[jj + 2], v);
            vec2 p = (1 - t) * (1 - t) * vertices[jj + 0]
                + 2 * (1 - t) * t * vertices[jj + 1]
                + t * t * vertices[jj + 2];
            float dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_dsqr = dsqr;
            }
            jj += 2;
        } else if (segments[ii] == cube) {
            float t = cube_closest_point(vertices[jj + 0], vertices[jj + 1], vertices[jj + 2], vertices[jj + 3], v);
            float s = 1.f - t;
            float t2 = t * t;
            float s2 = s * s;
            vec2 p = s2 * s * vertices[jj + 0]
                + 3.f * s2 * t * vertices[jj + 1]
                + 3.f * s * t2 * vertices[jj + 2]
                + t * t2 * vertices[jj + 3];
            float dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_dsqr = dsqr;
            }
            jj += 3;
        }
    }

    return best_idx;
}

//------------------------------------------------------------------------------
vec2 ship_editor::closest_point(std::vector<vec2> const& vertices, std::vector<segment_type> const& segments, vec2 v) const
{
    std::size_t best_idx = SIZE_MAX;
    vec2 best_point = vec2_zero;
    float best_dsqr = FLT_MAX;
    for (std::size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (segments[ii] == line) {
            float t = segment_closest_point(vertices[jj + 0], vertices[jj + 1], v);
            vec2 p = vertices[jj + 0] + (vertices[jj + 1] - vertices[jj + 0]) * t;
            float dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 1;
        } else if (segments[ii] == quad) {
            float t = quad_closest_point(vertices[jj + 0], vertices[jj + 1], vertices[jj + 2], v);
            vec2 p = (1 - t) * (1 - t) * vertices[jj + 0]
                   + 2 * (1 - t) * t * vertices[jj + 1]
                   + t * t * vertices[jj + 2];
            float dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 2;
        } else if (segments[ii] == cube) {
            float t = cube_closest_point(vertices[jj + 0], vertices[jj + 1], vertices[jj + 2], vertices[jj + 3], v);
            float s = 1.f - t;
            float t2 = t * t;
            float s2 = s * s;
            vec2 p = s2 * s * vertices[jj + 0]
                + 3.f * s2 * t * vertices[jj + 1]
                + 3.f * s * t2 * vertices[jj + 2]
                + t * t2 * vertices[jj + 3];
            float dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 3;
        }
    }

    return best_point;
}

//------------------------------------------------------------------------------
bool ship_editor::insert_vertex(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v)
{
    std::size_t best_idx = SIZE_MAX;
    float best_t = 0;
    vec2 best_point = vec2_zero;
    float best_dsqr = FLT_MAX;

    if (length_sqr(v - vertices[0]) < minimum_vertex_dsqr) {
        return false;
    }

    for (std::size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (segments[ii] == line) {
            if (length_sqr(v - vertices[jj + 1]) < minimum_vertex_dsqr) {
                return false;
            }
            float t = segment_closest_point(vertices[jj + 0], vertices[jj + 1], v);
            vec2 p = vertices[jj + 0] + (vertices[jj + 1] - vertices[jj + 0]) * t;
            float dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_t = t;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 1;
        } else if (segments[ii] == quad) {
            if (length_sqr(v - vertices[jj + 2]) < minimum_vertex_dsqr) {
                return false;
            }
            float t = quad_closest_point(vertices[jj + 0], vertices[jj + 1], vertices[jj + 2], v);
            vec2 p = (1 - t) * (1 - t) * vertices[jj + 0]
                + 2 * (1 - t) * t * vertices[jj + 1]
                + t * t * vertices[jj + 2];
            float dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_t = t;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 2;
        } else if (segments[ii] == cube) {
            if (length_sqr(v - vertices[jj + 3]) < minimum_vertex_dsqr) {
                return false;
            }
            float t = cube_closest_point(vertices[jj + 0], vertices[jj + 1], vertices[jj + 2], vertices[jj + 3], v);
            float s = 1.f - t;
            float t2 = t * t;
            float s2 = s * s;
            vec2 p = s2 * s * vertices[jj + 0]
                   + 3.f * s2 * t * vertices[jj + 1]
                   + 3.f * s * t2 * vertices[jj + 2]
                   + t * t2 * vertices[jj + 3];
            float dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_t = t;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 3;
        }
    }

    if (best_idx == SIZE_MAX) {
        return false;
    }

    for (std::size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (ii != best_idx) {
            if (segments[ii] == line) {
                jj += 1;
            } else if (segments[ii] == quad) {
                jj += 2;
            } else if (segments[ii] == cube) {
                jj += 3;
            }
            continue;
        }

        if (segments[ii] == line) {
            // convert the line to a bezier curve by inserting a control point
            vertices.insert(vertices.begin() + jj + 1, best_point);
            segments.insert(segments.begin() + ii, line);
            return true;
        }

        if (segments[ii] == quad) {
            // use De Casteljau's algorithm to subdivide the curve at `best_point`
            segments.insert(segments.begin() + ii, quad);
            vec2 p01 = (1.f - best_t) * vertices[jj + 0] + best_t * vertices[jj + 1];
            vec2 p12 = (1.f - best_t) * vertices[jj + 1] + best_t * vertices[jj + 2];
            // [a b c] -> [a p12 c]
            vertices[jj + 1] = p12;
            // [a p12 c] -> [a p01 p p12 c]
            vertices.insert(vertices.begin() + jj + 1, {p01, best_point});
            return true;
        }

        if (segments[ii] == cube) {
            // use De Casteljau's algorithm to subdivide the curve at `best_point`
            segments.insert(segments.begin() + ii, cube);
            vec2 p01 = (1.f - best_t) * vertices[jj + 0] + best_t * vertices[jj + 1];
            vec2 p12 = (1.f - best_t) * vertices[jj + 1] + best_t * vertices[jj + 2];
            vec2 p23 = (1.f - best_t) * vertices[jj + 2] + best_t * vertices[jj + 3];
            vec2 p012 = (1.f - best_t) * p01 + best_t * p12;
            vec2 p123 = (1.f - best_t) * p12 + best_t * p23;
            // [a b c d] -> [a p123 p23 d]
            vertices[jj + 1] = p123;
            vertices[jj + 2] = p23;
            // [a p123 p23 d] -> [a p01 p012 p p123 p23 d]
            vertices.insert(vertices.begin() + jj + 1, {p01, p012, best_point});
            return true;
        }
    }

    assert(false);
    return false;
}

//------------------------------------------------------------------------------
bool ship_editor::remove_vertex(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v)
{
    std::size_t best_idx = SIZE_MAX;
    float best_dsqr = FLT_MAX;
    for (std::size_t ii = 1; ii + 1 < vertices.size(); ++ii) {
        float dsqr = length_sqr(v - vertices[ii]);
        if (dsqr < best_dsqr) {
            best_idx = ii;
            best_dsqr = dsqr;
        }
    }

    if (best_dsqr > 1.f) {
        return false;
    }

    // cannot delete first or last vertex
    if (best_idx == 0 || best_idx >= vertices.size() - 1) {
        return false;
    }

    for (std::size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (segments[ii] == line) {
            // deleting first point of a line
            if (best_idx == jj + 0) {
                vertices.erase(vertices.begin() + best_idx);
                segments.erase(segments.begin() + ii);
                return true;
            }
            jj += 1;
        } else if (segments[ii] == quad) {
            // deleting first point of a quadratic bezier curve
            if (best_idx == jj + 0) {
                vertices.erase(vertices.begin() + best_idx, vertices.begin() + best_idx + 2);
                segments.erase(segments.begin() + ii);
                return true;

                // deleting quadratic bezier control point, turn into a line
            } else if (best_idx == jj + 1) {
                segments[ii] = line;
                vertices.erase(vertices.begin() + best_idx);
                return true;
            }
            jj += 2;
        } else if (segments[ii] == cube) {
            // deleting cubic bezier control point, turn into a quadratic bezier curve
            if (best_idx == jj + 1 || best_idx == jj + 2) {
                segments[ii] = quad;
                vertices.erase(vertices.begin() + best_idx);
                return true;
            }
            jj += 3;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
bool ship_editor::upconvert_segment(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v)
{
    std::size_t best_idx = closest_segment(vertices, segments, v);
    for (std::size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (ii != best_idx) {
            if (segments[ii] == line) {
                jj += 1;
            } else if (segments[ii] == quad) {
                jj += 2;
            } else if (segments[ii] == cube) {
                jj += 3;
            }
            continue;
        }

        if (segments[ii] == line) {
            vertices.insert(vertices.begin() + jj + 1, v);
            segments[ii] = quad;
        } else if (segments[ii] == quad) {
            vertices.insert(vertices.begin() + jj + 1, v);
            segments[ii] = cube;
        }

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool ship_editor::downconvert_segment(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v)
{
    std::size_t best_idx = closest_segment(vertices, segments, v);
    for (std::size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (ii != best_idx) {
            if (segments[ii] == line) {
                jj += 1;
            } else if (segments[ii] == quad) {
                jj += 2;
            } else if (segments[ii] == cube) {
                jj += 3;
            }
            continue;
        }

        if (segments[ii] == quad) {
            vertices.erase(vertices.begin() + jj + 1);
            segments[ii] = line;
        } else if (segments[ii] == cube) {
            vertices.erase(vertices.begin() + jj + 1);
            segments[ii] = quad;
        }

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool ship_editor::insert_turret(vec2 v)
{
    if (_turret_instance < _turret_instances.size()) {
        std::size_t index = _turret_instances[_turret_instance].index;
        if (insert_vertex(_turrets[index].vertices,
                          _turrets[index].segments,
                          v * _turret_instances[_turret_instance].transform.inverse_transform())) {
            return true;
        }
    }
    _turret_instances.push_back({mat3::transform(v, rot2_identity), 0});
    if (!_turrets.size()) {
        _turrets.push_back({5.25f});
        _turrets.back().vertices = {vec2(-5,0), vec2(0,5), vec2(5,0)};
        _turrets.back().segments = {line, line};
    }
    return true;
}

//------------------------------------------------------------------------------
bool ship_editor::remove_turret(vec2 v)
{
    (void)v;
    return false;
}

//------------------------------------------------------------------------------
bool ship_editor::key_event(int key, bool down)
{
    if (down) {
        if (key == K_MOUSE2) {
            if (_control) {
                _is_panning_image = true;
            } else {
                _is_panning = true;
            }
            return true;
        }
        if (key == K_CTRL) {
            _control = true;
        }
    } else if (key == K_MOUSE2) {
        if (_is_panning) {
            _is_panning = false;
            return true;
        } if (_is_panning_image) {
            _is_panning_image = false;
            return true;
        }
    } else {
        if (key == K_CTRL) {
            _control = false;
        } else if (key == K_MOUSE1) {
            _is_dragging = false;
        }
    }

    if (!down) {
        return false;
    }

    switch (key) {
        case '[':
            _snap_distance *= 2.f;
            return true;

        case ']':
            _snap_distance *= .5f;
            return true;

        case 'd':
            _mode = editor_mode::deck;
            return true;

        case 'g':
            _draw_grid = !_draw_grid;
            return true;

        case 's':
            _snap_to_grid = !_snap_to_grid;
            return true;

        case 't':
            _mode = editor_mode::turret;
            return true;

        case 'l':
            _draw_linearized = !_draw_linearized;
            if (_draw_linearized) {
                _deck_linearized = linearize(_deck_vertices, _deck_segments);
                for (std::size_t ii = 0; ii < _turrets.size(); ++ii) {
                    _turrets[ii].linearized = linearize(_turrets[ii].vertices, _turrets[ii].segments);
                }
            }
            return true;

        case K_F5:
            save("editor.dat");
            return true;

        case K_F8:
            load("editor.dat");
            return true;

        case K_F9:
            export_verts("editor.log");
            return true;

        case K_MOUSE1: {
            if (_mode == editor_mode::deck) {
                vec2 p = cursor_to_world();
                std::size_t best_idx = closest_vertex(_deck_vertices, p);
                if (best_idx < _deck_vertices.size() && length_sqr(_deck_vertices[best_idx] - p) < minimum_vertex_dsqr) {
                    _is_dragging = true;
                    _drag_index = best_idx;
                    return true;
                }
            } else if (_mode == editor_mode::turret) {
                if (_turret_instance < _turret_instances.size()) {
                    vec2 p = cursor_to_world() * _turret_instances[_turret_instance].transform.inverse_transform();
                    std::vector<vec2> const& v = _turrets[_turret_instances[_turret_instance].index].vertices;
                    std::size_t best_idx = closest_vertex(v, p);
                    float best_dsqr = best_idx < v.size() ? length_sqr(v[best_idx] - p) : FLT_MAX;
                    float center_dsqr = p.length_sqr();
                    if (center_dsqr < best_dsqr && center_dsqr < minimum_vertex_dsqr) {
                        _is_dragging = true;
                        _drag_index = SIZE_MAX;
                        return true;
                    } else if (best_dsqr < minimum_vertex_dsqr) {
                        _is_dragging = true;
                        _drag_index = best_idx;
                        return true;
                    }
                }
            }
            break;
        }

        case K_MOUSE2:
            break;

        case K_MWHEELUP:
            _view.size /= 1.25f;
            return true;

        case K_MWHEELDOWN:
            _view.size *= 1.25f;
            return true;

        case K_INS:
            if (_mode == editor_mode::deck) {
                insert_vertex(_deck_vertices, _deck_segments, cursor_to_world());
            } else if (_mode == editor_mode::turret) {
                insert_turret(cursor_to_world());
            }
            return true;

        case K_DEL:
            if (_mode == editor_mode::deck) {
                remove_vertex(_deck_vertices, _deck_segments, cursor_to_world());
            } else if (_mode == editor_mode::turret) {
                remove_turret(cursor_to_world());
            }
            return true;

        case K_PGUP:
            if (_mode == editor_mode::deck) {
                return upconvert_segment(_deck_vertices, _deck_segments, cursor_to_world());
            } else if (_mode == editor_mode::turret) {
                if (_turret_instance < _turret_instances.size()) {
                    return upconvert_segment(
                        _turrets[_turret_instances[_turret_instance].index].vertices,
                        _turrets[_turret_instances[_turret_instance].index].segments,
                        cursor_to_world() * _turret_instances[_turret_instance].transform.inverse_transform());
                }
            }
            break;

        case K_PGDN:
            if (_mode == editor_mode::deck) {
                return downconvert_segment(_deck_vertices, _deck_segments, cursor_to_world());
            } else if (_mode == editor_mode::turret) {
                if (_turret_instance < _turret_instances.size()) {
                    return downconvert_segment(
                        _turrets[_turret_instances[_turret_instance].index].vertices,
                        _turrets[_turret_instances[_turret_instance].index].segments,
                        cursor_to_world() * _turret_instances[_turret_instance].transform.inverse_transform());
                }
            }
            break;

    }

    return false;
}

//------------------------------------------------------------------------------
void ship_editor::cursor_event(vec2 position)
{
    if (_is_panning) {
        _view.origin -= (position - _cursor) * _view.size;
    } else if (_is_panning_image) {
        _image_offset = (position - _cursor) * _view.size + _image_offset;
    } else if (_is_dragging) {
        if (_mode == editor_mode::deck) {
            _deck_vertices[_drag_index] += (position - _cursor) * _view.size;
        } else if (_mode == editor_mode::turret) {
            if (_turret_instance < _turret_instances.size()) {
                vec2 delta = (vec3((position - _cursor) * _view.size) * _turret_instances[_turret_instance].transform.inverse_transform()).to_vec2();
                if (_drag_index < _turrets[_turret_instances[_turret_instance].index].vertices.size()) {
                    _turrets[_turret_instances[_turret_instance].index].vertices[_drag_index] += delta;
                } else {
                    vec2 pos = vec2_zero * _turret_instances[_turret_instance].transform;
                    pos += (position - _cursor) * _view.size;
                    _turret_instances[_turret_instance].transform[2][0] = pos.x;
                    _turret_instances[_turret_instance].transform[2][1] = pos.y;
                }
            }
        }
    }
    _cursor = position;
}

//------------------------------------------------------------------------------
struct file_header
{
    std::size_t header_size;
    std::size_t image_name_size;
    std::size_t image_name_offset;
    vec2 image_offset;
    float image_scale;
    std::size_t deck_vertices_size;
    std::size_t deck_vertices_offset;
    std::size_t deck_segments_size;
    std::size_t deck_segments_offset;
};

//------------------------------------------------------------------------------
void ship_editor::save(string::view filename) const
{
    g_Game->message("saving '%s'...\n", filename.c_str());

    file::stream s = file::open(filename, file::mode::write);
    if (!s) {
        return;
    }

    file_header h;

    h.header_size = sizeof(file_header);
    h.image_name_size = _image->name().length();
    h.image_name_offset = 0;
    h.image_offset = _image_offset;
    h.image_scale = _image_scale;
    h.deck_vertices_size = _deck_vertices.size() * sizeof(_deck_vertices[0]);
    h.deck_vertices_offset = 0;
    h.deck_segments_size = _deck_segments.size() * sizeof(_deck_segments[0]);
    h.deck_segments_offset = 0;

    // write header placeholder
    s.write((file::byte const*)&h, h.header_size);
    // write image name
    h.image_name_offset = s.tell();
    s.write((file::byte const*)_image->name().c_str(), h.image_name_size);
    // write deck vertices
    h.deck_vertices_offset = s.tell();
    s.write((file::byte const*)_deck_vertices.data(), h.deck_vertices_size);
    // write deck segments
    h.deck_segments_offset = s.tell();
    s.write((file::byte const*)_deck_segments.data(), h.deck_segments_size);
    // rewrite the file header with populated offset fields
    s.seek(0, file::seek::set);
    s.write((file::byte const*)&h, h.header_size);

    s.close();
}

//------------------------------------------------------------------------------
bool ship_editor::load(string::view filename)
{
    g_Game->message("loading '%s'...\n", filename.c_str());

    file::buffer b = file::read(filename);
    if (!b.size()) {
        return false;
    }

    file_header const* h = reinterpret_cast<file_header const*>(b.data());

    string::view image_name(
        (char const*)b.data() + h->image_name_offset,
        (char const*)b.data() + h->image_name_offset + h->image_name_size);
    _image = application::singleton()->window()->renderer()->load_image(image_name);

    _image_offset = h->image_offset;
    _image_scale = h->image_scale;

    _deck_vertices.resize(h->deck_vertices_size / sizeof(_deck_vertices[0]));
    memcpy(_deck_vertices.data(), b.data() + h->deck_vertices_offset, h->deck_vertices_size);

    _deck_segments.resize(h->deck_segments_size / sizeof(_deck_segments[0]));
    memcpy(_deck_segments.data(), b.data() + h->deck_segments_offset, h->deck_segments_size);

    _deck_linearized = linearize(_deck_vertices, _deck_segments);

    return true;
}

//------------------------------------------------------------------------------
void ship_editor::export_verts(string::view filename) const
{
    g_Game->message("exporting '%s'...\n", filename.c_str());

    file::stream s = file::open(filename, file::mode::write);
    if (!s) {
        return;
    }

    s.printf("{\n    ");
    for (std::size_t ii = 0; ii < _deck_linearized.size(); ++ii) {
        s.printf("vec2(%.2ff, %.2ff), ", _deck_linearized[ii].x, _deck_linearized[ii].y);
    }
    s.printf("\n}\n");
    for (std::size_t jj = 0; jj < _turrets.size(); ++jj) {
        s.printf("{\n    ");
        for (std::size_t ii = 0; ii < _turrets[jj].linearized.size(); ++ii) {
            s.printf("vec2(%.2ff, %.2ff), ", _turrets[jj].linearized[ii].x, _turrets[jj].linearized[ii].y);
        }
        s.printf("\n}\n");
    }
}

//------------------------------------------------------------------------------
std::vector<vec2> ship_editor::linearize(std::vector<vec2> const& vertices, std::vector<segment_type> const& segments)
{
    std::vector<vec2> linearized;

    linearized.push_back(vertices[0]);

    for (std::size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (segments[ii] == line) {
            linearized.push_back(vertices[jj + 1]);
            jj += 1;
        } else if (segments[ii] == quad) {
            std::vector<vec2> v = subdivide([&](float t){
                float s = 1.f - t;
                return s * s * vertices[jj + 0]
                    + 2.f * s * t * vertices[jj + 1]
                    + t * t * vertices[jj + 2];
                }, 0.1f);
            linearized.insert(linearized.end(), v.begin() + 1, v.end());
            jj += 2;
        } else if (segments[ii] == cube) {
            std::vector<vec2> v = subdivide([&](float t){
                float s = 1.f - t;
                float t2 = t * t;
                float s2 = s * s;
                return s2 * s * vertices[jj + 0]
                     + 3.f * s2 * t * vertices[jj + 1]
                     + 3.f * s * t2 * vertices[jj + 2]
                     + t * t2 * vertices[jj + 3];
                }, 0.1f);
            linearized.insert(linearized.end(), v.begin() + 1, v.end());
            jj += 3;
        }
    }

    // duplicate vertices along the bottom half
    for (std::size_t ii = linearized.size() - 2; ii > 0; --ii) {
        linearized.push_back(vec2(linearized[ii].x, -linearized[ii].y));
    }

    // duplicate final vertex if necessary, i.e. for transom stern
    if (linearized[0].y) {
        linearized.push_back(vec2(linearized[0].x, -linearized[0].y));
    }

    return linearized;
}

//------------------------------------------------------------------------------
std::vector<vec2> ship_editor::subdivide(std::function<vec2(float)> fn, float error)
{
    std::vector<vec2> p;
    std::vector<float> t;
    std::vector<float> e;

    p.push_back(fn(0.f));
    p.push_back(fn(1.f));
    t.push_back(0.f);
    t.push_back(1.f);
    e.push_back(0.f);

    for (std::size_t n = 1; ; ++n) {
        p.insert(p.end() - 1, vec2_zero);
        t.insert(t.end() - 1, 0.f);
        e.push_back(0.f);

        for (std::size_t ii = 1; ii < n + 1; ++ii) {
            t[ii] = float(ii) / float(n + 1);
            p[ii] = fn(t[ii]);
        }

        for (std::size_t jj = 0; jj < 128; ++jj) {
            float rms = 0.f;
            float emax = 0.f;
            for (std::size_t ii = 0; ii < n + 1; ++ii) {
                float t0 = .5f * (t[ii] + t[ii + 1]);
                vec2 p0 = fn(t0);
                vec2 v = p[ii + 1] - p[ii];
                vec2 r = (p[ii] - p0) - dot(p[ii] - p0, v) * v / length_sqr(v);
                e[ii] = length(r);
                emax = max(emax, e[ii]);
                rms += square(e[ii]);
            }

            if (emax < error) {
                return p;
            }

            rms = sqrt(rms / float(n));
            if (rms > 8.f * error) {
                break;
            }

            e[0] = 1.f / e[0];
            for (std::size_t ii = 1; ii < n + 1; ++ii) {
                e[ii] = e[ii - 1] + 1.f / e[ii];
            }
            for (std::size_t ii = 1; ii < n + 1; ++ii) {
                t[ii] = .5f * e[ii - 1] / e.back() + .5f * t[ii];
                p[ii] = fn(t[ii]);
            }
        }
    }
}

} // namespace game
