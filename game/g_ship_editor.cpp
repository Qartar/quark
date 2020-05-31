// g_ship_editor.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_keys.h"
#include "g_ship_editor.h"

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
{
    _view.size = {64.f, 48.f};
    //_view.viewport = {0, 0, 640, 480};
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

    if (_snap_to_mirror && _render_verts.size()) {
        out = _render_verts[0] * vec2(1,-1);

        for (auto const& v : _render_verts) {
            vec2 m(v.x, -v.y);
            float dsqr = (m - pos).length_sqr();
            if (dsqr < (out - pos).length_sqr() && dsqr < square(_snap_distance)) {
                out = m;
            }
        }

        if (_snap_to_grid && (grid_snap - pos).length_sqr() < (out - pos).length_sqr()) {
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
    for (vec2 v : _render_verts) {
        renderer->draw_box(vertex_size, v, color4(1,0,0,1));
    }

    vec2 world_pos = snap_vertex(cursor_to_world());

    if (_mode == editor_mode::vertices) {
        renderer->draw_box(vertex_size, world_pos, color4(1,1,0,1));
        if (_mirror) {
            renderer->draw_box(vertex_size, vec2(world_pos.x, -world_pos.y), color4(0,1,1,1));
        }
    }

    {
        string::view s = va("(%g, %g)", world_pos.x, world_pos.y);
        vec2 size = renderer->string_size(s);
        renderer->draw_string(s, _view.origin + _view.size * .5f - size, color4(1,1,1,.5f));
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
            _snap_distance *= .5f;
            return true;

        case ']':
            _snap_distance *= 2.f;
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

        case K_MOUSE1:
            if (_mode == editor_mode::vertices) {
                vec2 v = snap_vertex(cursor_to_world());
                _render_verts.push_back(v);
                if (_mirror) {
                    _render_verts.push_back(vec2(v.x, -v.y));
                }
                return true;
            }
            break;
        case K_MOUSE2:
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
