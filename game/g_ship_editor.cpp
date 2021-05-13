// g_ship_editor.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_keys.h"
#include "g_ship_editor.h"
#include "r_image.h"
#include "r_window.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
ship_editor::ship_editor()
    : _view{}
    , _cursor{}
    , _mode(editor_mode::vertices)
    , _snap_distance(1.f)
    , _snap_to_grid(true)
    , _snap_to_edge(true)
    , _snap_to_mirror(true)
    , _mirror(true)
    , _image(nullptr)
    , _image_offset(vec2_zero)
    , _image_scale(1.f,1.f)
    , _image_rotation(0.f)
    , _triangle_size(0)
{
    _view.viewport.maxs() = application::singleton()->window()->size();
    _view.size = {64.f, 64.f * float(_view.viewport.maxs().y) / float(_view.viewport.maxs().x)};

    _guide_vertices = {
        /*  0 */ {-12.5f,   6.f},
        /*  1 */ {-10.5f,  11.f},
        /*  2 */ { -9.f,   13.f},
        /*  3 */ { -2.f,   13.f},
        /*  4 */ {  1.5f,  12.f},
        /*  5 */ {  1.5f,  11.f},
        /*  6 */ { -2.5f,  11.f},
        /*  7 */ { -2.f,    7.f},
        /*  8 */ {  1.5f,   6.5f},
        /*  9 */ {  8.5f,   4.5f},
        /* 10 */ {  8.5f,   3.f},
        /* 11 */ { 12.f,    3.f},
        /* 12 */ { 12.f,    7.f},
        /* 13 */ { 15.f,    7.f},
        /* 14 */ { 16.5f,   5.5f},
        /* 15 */ { 18.f,    2.f},
        /* 16 */ { 18.f,   -2.f},
        /* 17 */ { 16.5f,  -5.5f},
        /* 18 */ { 15.f,   -7.f},
        /* 19 */ { 12.f,   -7.f},
        /* 20 */ { 12.f,   -3.f},
        /* 21 */ {  8.5f,  -3.f},
        /* 22 */ {  8.5f,  -4.5f},
        /* 23 */ {  1.5f,  -6.5f},
        /* 24 */ { -2.f,   -7.f},
        /* 25 */ { -2.5f, -11.f},
        /* 26 */ {  1.5f, -11.f},
        /* 27 */ {  1.5f, -12.f},
        /* 28 */ { -2.f,  -13.f},
        /* 29 */ { -9.f,  -13.f},
        /* 30 */ {-10.5f, -11.f},
        /* 31 */ {-12.5f,  -6.f},
    };

    _guide_triangles = {
         0,  1,  7,
         1,  6,  7,
         1,  2,  6,
         2,  3,  6,
         3,  4,  5,
         3,  5,  6,
         0,  7, 24,
         0, 24, 31,
         7,  8, 23,
         7, 23, 24,
         8, 10, 21,
         8, 21, 23,
         8,  9, 10,
        21, 22, 23,
        10, 11, 20,
        10, 20, 21,
        11, 12, 13,
        11, 13, 14,
        11, 14, 15,
        11, 15, 16,
        11, 16, 20,
        20, 19, 18,
        20, 18, 17,
        20, 17, 16,
        31, 30, 24,
        30, 25, 24,
        30, 29, 25,
        29, 28, 25,
        28, 27, 26,
        28, 26, 25,
    };

    _image = application::singleton()->window()->renderer()->load_image(
        //"C:\\Users\\Carter\\OneDrive\\Pictures\\Trade Wars\\Constellation.bmp"
        "C:\\Users\\Carter\\OneDrive\\Pictures\\Trade Wars\\T'Khasi_Orion.bmp"
    );
    _image_scale = vec2(1.5f / 9.f);
    _image_rotation = math::pi<float> / 2.f;
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
std::size_t ship_editor::insert_vertex(vec2 pos)
{
    for (std::size_t ii = 0; ii < _render_vertices.size(); ++ii) {
        if (_render_vertices[ii] == pos) {
            return ii;
        }
    }
    _render_vertices.push_back(pos);
    return _render_vertices.size() - 1;
}

//------------------------------------------------------------------------------
bool ship_editor::remove_vertex(vec2 pos)
{
    for (std::size_t ii = 0; ii < _render_vertices.size(); ++ii) {
        if (_render_vertices[ii] == pos) {
            return remove_vertex_by_index(ii);
        }
    }

    return false;
}

//------------------------------------------------------------------------------
bool ship_editor::remove_vertex_by_index(std::size_t vtx)
{
    // Remove the vertex, preserve order
    _render_vertices.erase(_render_vertices.begin() + vtx);

    // Remove any triangles containing the removed vertex
    std::size_t jj = 0;
    for (std::size_t ii = 0; ii < _render_triangles.size(); ii += 3) {
        if (!(_render_triangles[ii + 0] == vtx
            || _render_triangles[ii + 1] == vtx
            || _render_triangles[ii + 2] == vtx)) {
            _render_triangles[jj++] = _render_triangles[ii + 0];
            _render_triangles[jj++] = _render_triangles[ii + 1];
            _render_triangles[jj++] = _render_triangles[ii + 2];
        }
    }
    _render_triangles.resize(jj);

    // Update vertex indices on remaining triangles
    for (std::size_t ii = 0; ii < _render_triangles.size(); ++ii) {
        if (_render_triangles[ii] > vtx) {
            --_render_triangles[ii];
        }
    }

    return true;
}

//------------------------------------------------------------------------------
vec2 ship_editor::snap_vertex(vec2 pos) const
{
    vec2 out = pos;

    vec2 grid_snap = {
        std::floor(pos.x / _snap_distance + .5f) * _snap_distance,
        std::floor(pos.y / _snap_distance + .5f) * _snap_distance,
    };

    if (_snap_to_mirror && _render_vertices.size()) {
        vec2 mirror_snap = _render_vertices[0] * vec2(1,-1);

        for (auto const& v : _render_vertices) {
            vec2 m(v.x, -v.y);
            float dsqr = (m - pos).length_sqr();
            if (dsqr < (mirror_snap - pos).length_sqr()) {
                mirror_snap = m;
            }
        }

        if ((mirror_snap - pos).length_sqr() < _snap_distance) {
            out = mirror_snap;
        }

        if (_snap_to_grid && (grid_snap - pos).length_sqr() < (mirror_snap - pos).length_sqr()) {
            out = grid_snap;
        }
    } else if (_snap_to_grid) {
        out = grid_snap;
    }

    return out;
}

//------------------------------------------------------------------------------
void ship_editor::draw(render::system* renderer, time_value /*time*/) const
{
    if (_image) {
        render::view image_view = _view;
        image_view.angle = -_image_rotation;

        renderer->set_view(image_view);
        vec2 image_size = vec2(vec2i(_image->width(), _image->height())) * _image_scale;
        renderer->draw_image(_image, _image_offset - .5f * image_size, image_size, color4(1,1,1,1));
    }

    renderer->set_view(_view);

    {
        color4 c(1.f, 1.f, 1.f, .2f);
        for (float x = 0.f; x < .5f * _view.size.x; x += _snap_distance) {
            renderer->draw_line(_view.origin + vec2(x, _view.size.y), _view.origin + vec2(x, -_view.size.y), c, c);
            renderer->draw_line(_view.origin + vec2(-x, _view.size.y), _view.origin + vec2(-x, -_view.size.y), c, c);
        }
        for (float y = 0.f; y < .5f * _view.size.x; y += _snap_distance) {
            renderer->draw_line(_view.origin + vec2(_view.size.x, y), _view.origin + vec2(-_view.size.x, y), c, c);
            renderer->draw_line(_view.origin + vec2(_view.size.x, -y), _view.origin + vec2(-_view.size.x, -y), c, c);
        }
    }

    vec2 vertex_size = vec2(_view.size.y * (1.f / 384.f));
    for (vec2 v : _guide_vertices) {
        renderer->draw_box(vertex_size, v, color4(1,1,1,1));
    }

    for (size_t ii = 0; ii < _guide_triangles.size(); ii += 3) {
        vec2 v0 = _guide_vertices[_guide_triangles[ii + 0]];
        vec2 v1 = _guide_vertices[_guide_triangles[ii + 1]];
        vec2 v2 = _guide_vertices[_guide_triangles[ii + 2]];
        renderer->draw_line(v0, v1, color4(1,1,1,.5f), color4(1,1,1,.5f));
        renderer->draw_line(v1, v2, color4(1,1,1,.5f), color4(1,1,1,.5f));
        renderer->draw_line(v2, v0, color4(1,1,1,.5f), color4(1,1,1,.5f));
    }

    for (vec2 v : _render_vertices) {
        renderer->draw_box(vertex_size, v, color4(1,0,0,1));
    }

    for (size_t ii = 0; ii < _render_triangles.size(); ii += 3) {
        vec2 v0 = _render_vertices[_render_triangles[ii + 0]];
        vec2 v1 = _render_vertices[_render_triangles[ii + 1]];
        vec2 v2 = _render_vertices[_render_triangles[ii + 2]];
        renderer->draw_line(v0, v1, color4(1,0,0,.5f), color4(1,0,0,.5f));
        renderer->draw_line(v1, v2, color4(1,0,0,.5f), color4(1,0,0,.5f));
        renderer->draw_line(v2, v0, color4(1,0,0,.5f), color4(1,0,0,.5f));
    }

    vec2 world_pos = snap_vertex(cursor_to_world());

    if (_mode == editor_mode::vertices) {
        renderer->draw_box(vertex_size, world_pos, color4(1,1,0,1));
        if (_mirror) {
            renderer->draw_box(vertex_size, vec2(world_pos.x, -world_pos.y), color4(0,1,1,1));
        }
    } else if (_mode == editor_mode::triangles) {
        renderer->draw_box(vertex_size, world_pos, color4(1,1,0,1));
        if (_mirror) {
            renderer->draw_box(vertex_size, vec2(world_pos.x, -world_pos.y), color4(0,1,1,1));
        }

        if (_triangle_size > 0) {
            renderer->draw_box(vertex_size, _render_vertices[_triangle[0]], color4(1,1,0,1));
            renderer->draw_line(world_pos, _render_vertices[_triangle[0]], color4(1,1,0,.5f), color4(1,1,0,.5f));
            if (_mirror) {
                renderer->draw_line(vec2(world_pos.x, -world_pos.y), _render_vertices[_triangle_mirror[0]], color4(0,1,1,.5f), color4(0,1,1,.5f));
            }
        }
        if (_triangle_size > 1) {
            renderer->draw_box(vertex_size, _render_vertices[_triangle[1]], color4(1,1,0,1));
            renderer->draw_line(world_pos, _render_vertices[_triangle[1]], color4(1,1,0,.5f), color4(1,1,0,.5f));
            renderer->draw_line(_render_vertices[_triangle[0]], _render_vertices[_triangle[1]], color4(1,1,0,.5f), color4(1,1,0,.5f));
            if (_mirror) {
                renderer->draw_box(vertex_size, _render_vertices[_triangle_mirror[1]], color4(0,1,1,1));
                renderer->draw_line(vec2(world_pos.x, -world_pos.y), _render_vertices[_triangle_mirror[1]], color4(0,1,1,.5f), color4(0,1,1,.5f));
                renderer->draw_line(_render_vertices[_triangle_mirror[0]], _render_vertices[_triangle_mirror[1]], color4(0,1,1,.5f), color4(0,1,1,.5f));
            }
        }
    }

    {
        string::view s = va("(%g, %g)", world_pos.x, world_pos.y);
        vec2 size = renderer->string_size(s);
        renderer->draw_string(s, _view.origin + _view.size * .5f - size, color4(1,1,1,.5f));
    }
    {
        vec2 size = renderer->monospace_size("foo");
        renderer->draw_monospace("(s) snap to grid", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*1.f), color4(1,1,1, _snap_to_grid ? .6f : .3f));
        renderer->draw_monospace("( ) snap to edge", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*2.f), color4(1,1,1, _snap_to_edge ? .6f : .3f));
        renderer->draw_monospace("( ) snap to mirror", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*3.f), color4(1,1,1, _snap_to_mirror ? .6f : .3f));
        renderer->draw_monospace("(m) mirror", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*4.f), color4(1,1,1, _mirror ? .6f : .3f));

        renderer->draw_monospace("(v) edit vertices", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*6.f), color4(1,1,1, _mode == editor_mode::vertices ? .6f : .3f));
        renderer->draw_monospace("(t) edit triangles", _view.origin - _view.size * vec2(.5f,-.5f) - vec2(0,size.y*7.f), color4(1,1,1, _mode == editor_mode::triangles ? .6f : .3f));
    }
}

//------------------------------------------------------------------------------
bool ship_editor::key_event(int key, bool down)
{
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

        case 'm':
            _mirror = !_mirror;
            return true;

        case 's':
            _snap_to_grid = !_snap_to_grid;
            return true;

        case 'v':
            if (_mode == editor_mode::vertices) {
                _mode = editor_mode::none;
            } else {
                _mode = editor_mode::vertices;
            }
            return true;

        case 't':
            if (_mode == editor_mode::triangles) {
                _mode = editor_mode::none;
            } else {
                _triangle_size = 0;
                _mode = editor_mode::triangles;
            }
            return true;

        case K_MOUSE1:
            if (_mode == editor_mode::vertices) {
                vec2 v = snap_vertex(cursor_to_world());
                insert_vertex(v);
                if (_mirror) {
                    insert_vertex(vec2(v.x, -v.y));
                }
                return true;
            } else if (_mode == editor_mode::triangles) {
                vec2 v = snap_vertex(cursor_to_world());
                std::size_t idx = insert_vertex(v);
                if (_triangle_size == 2) {
                    _render_triangles.push_back(_triangle[0]);
                    _render_triangles.push_back(_triangle[1]);
                    _render_triangles.push_back(idx);
                } else {
                    _triangle[_triangle_size] = idx;
                }
                if (_mirror) {
                    std::size_t mirror = insert_vertex(vec2(v.x, -v.y));
                    if (_triangle_size == 2) {
                        _render_triangles.push_back(_triangle_mirror[0]);
                        _render_triangles.push_back(_triangle_mirror[1]);
                        _render_triangles.push_back(mirror);
                    } else {
                        _triangle_mirror[_triangle_size] = mirror;
                    }
                }
                _triangle_size = (_triangle_size + 1) % 3;
            }
            break;

        case K_MOUSE2:
            if (_mode == editor_mode::vertices) {
                vec2 v = snap_vertex(cursor_to_world());
                if (remove_vertex(v)) {
                    if (_mirror) {
                        remove_vertex(vec2(v.x, -v.y));
                    }
                    return true;
                }
            } else if (_mode == editor_mode::triangles && _triangle_size) {
                --_triangle_size;
                return true;
            }
            break;
    }

    return false;
}

//------------------------------------------------------------------------------
void ship_editor::cursor_event(vec2 position)
{
    _cursor = position;
}

} // namespace game
