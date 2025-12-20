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
    , _image_scale(1.f/10.f)
{
    _view.viewport.maxs() = application::singleton()->window()->size();
    _view.size = {64.f, 64.f * float(_view.viewport.maxs().y) / float(_view.viewport.maxs().x)};

    _image = application::singleton()->window()->renderer()->load_image(
        //"C:\\Users\\Carter\\OneDrive\\Pictures\\Trade Wars\\Constellation.bmp"
        "D:\\Users\\Carter\\Pictures\\Yamato1945.bmp"
    );

    _deck_vertices = { SHIP(263.f, 39.f) };
    _deck_segments = { quad, quad };
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
        vec2 image_size = vec2(vec2i(_image->width(), -_image->height())) * _image_scale;
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

    vec2 vertex_size = vec2(_view.size.y * (1.f / 384.f));

    //
    // draw deck outline
    //

    if (_draw_linearized) {
        for (size_t ii = 0; ii + 1 < _deck_linearized.size(); ++ii) {
            renderer->draw_line(_deck_linearized[ii + 0], _deck_linearized[ii + 1], color4(1,1,1,1), color4(1,1,1,1));
            renderer->draw_box(vertex_size, _deck_linearized[ii + 0], color4(1,0,0,1));
            renderer->draw_box(vertex_size, _deck_linearized[ii + 1], color4(1,0,0,1));
        }
        if (_deck_linearized.size()) {
            renderer->draw_line(_deck_linearized.back(), _deck_linearized.front(), color4(1,1,1,1), color4(1,1,1,1));
            renderer->draw_box(vertex_size, _deck_linearized.back(), color4(1,0,0,1));
            renderer->draw_box(vertex_size, _deck_linearized.front(), color4(1,0,0,1));
        }
    } else {
        for (size_t ii = 0, jj = 0; ii < _deck_segments.size(); ++ii) {
            if (_deck_segments[ii] == line) {
                renderer->draw_line(_deck_vertices[jj + 0], _deck_vertices[jj + 1], color4(1,1,1,1), color4(1,1,1,1));
                renderer->draw_box(vertex_size, _deck_vertices[jj + 0], color4(1,0,0,1));
                renderer->draw_box(vertex_size, _deck_vertices[jj + 1], color4(1,0,0,1));
                jj += 1;
            } else if (_deck_segments[ii] == quad) {
                draw_bezier(renderer, _deck_vertices[jj + 0], _deck_vertices[jj + 1], _deck_vertices[jj + 2], color4(1,1,1,1));
                renderer->draw_box(vertex_size, _deck_vertices[jj + 0], color4(1,0,0,1));
                renderer->draw_box(vertex_size, _deck_vertices[jj + 1], color4(1,1,0,1));
                renderer->draw_box(vertex_size, _deck_vertices[jj + 2], color4(1,0,0,1));
                jj += 2;
            }
        }
    }

    //
    // draw vertex highlight and closest point
    //

    {
        std::size_t best_idx = SIZE_MAX;
        float best_dsqr = 1.f;
        vec2 p = cursor_to_world();
        for (std::size_t ii = 0; ii < _deck_vertices.size(); ++ii) {
            float dsqr = length_sqr(_deck_vertices[ii] - p);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_dsqr = dsqr;
            }
        }
        if (best_idx != SIZE_MAX) {
            renderer->draw_box(vertex_size, _deck_vertices[best_idx], color4(0,1,0,1));
            renderer->draw_line(vec2(_deck_vertices[best_idx].x, vmin.y), vec2(_deck_vertices[best_idx].x, vmax.y), color4(0,1,0,.1f), color4(0,1,0,.1f));
            renderer->draw_line(vec2(vmin.x, _deck_vertices[best_idx].y), vec2(vmax.x, _deck_vertices[best_idx].y), color4(0,1,0,.1f), color4(0,1,0,.1f));
        } else {
            vec2 v = closest_point(_deck_vertices, _deck_segments, cursor_to_world());
            renderer->draw_box(vertex_size, v, color4(0,1,1,1));
            renderer->draw_line(vec2(v.x, vmin.y), vec2(v.x, vmax.y), color4(0,1,1,.1f), color4(0,1,1,.1f));
            renderer->draw_line(vec2(vmin.x, v.y), vec2(vmax.x, v.y), color4(0,1,1,.1f), color4(0,1,1,.1f));
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
    }
}

//------------------------------------------------------------------------------
void ship_editor::draw_bezier(render::system* renderer, vec2 a, vec2 b, vec2 c, color4 color) const
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
        }
    }

    if (best_idx == SIZE_MAX) {
        return false;
    }

    for (std::size_t ii = 0, jj = 0; ii < segments.size(); ++ii) {
        if (ii != best_idx) {
            jj += segments[ii] == line ? 1 : 2;
            continue;
        }

        if (segments[ii] == line) {
            // convert the line to a bezier curve by inserting a control point
            segments[ii] = quad;
            vertices.insert(vertices.begin() + jj + 1, best_point);
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
        }
    }

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

        case 'g':
            _draw_grid = !_draw_grid;
            return true;

        case 's':
            _snap_to_grid = !_snap_to_grid;
            return true;

        case 'l':
            _draw_linearized = !_draw_linearized;
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
            std::size_t best_idx = SIZE_MAX;
            float best_dsqr = 1.f;
            vec2 p = cursor_to_world();
            for (std::size_t ii = 0; ii < _deck_vertices.size(); ++ii) {
                float dsqr = length_sqr(_deck_vertices[ii] - p);
                if (dsqr < best_dsqr) {
                    best_idx = ii;
                    best_dsqr = dsqr;
                }
            }
            if (best_idx != SIZE_MAX) {
                _is_dragging = true;
                _drag_index = best_idx;
                return true;
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
            insert_vertex(_deck_vertices, _deck_segments, cursor_to_world());
            return true;

        case K_DEL:
            remove_vertex(_deck_vertices, _deck_segments, cursor_to_world());
            return true;

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
        _deck_vertices[_drag_index] += (position - _cursor) * _view.size;
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

    s.printf("{ ");
    for (std::size_t ii = 0; ii < _deck_linearized.size(); ++ii) {
        s.printf("vec2(%.2ff, %.2ff), ", _deck_linearized[ii].x, _deck_linearized[ii].y);
    }
    s.printf("}\n");
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
            // TODO: subdivide bezier curve into multiple line segments based on error
            linearized.push_back(vertices[jj + 2]);
            jj += 2;
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

} // namespace game
