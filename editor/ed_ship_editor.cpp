// ed_ship_editor.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_keys.h"
#include "ed_ship_editor.h"
#include "r_image.h"
#include "r_window.h"

#include <algorithm>
#include <iterator>

////////////////////////////////////////////////////////////////////////////////
namespace editor {

namespace {

std::vector<std::size_t> sorted_union(std::vector<std::size_t> const& a, std::vector<std::size_t> const& b)
{
    std::vector<std::size_t> out;
    out.reserve(a.size() + b.size());
    std::set_union(a.begin(), a.end(),
                   b.begin(), b.end(),
                   std::back_inserter(out));

    return out;
}

std::vector<std::size_t> sorted_difference(std::vector<std::size_t> const& a, std::vector<std::size_t> const& b)
{
    std::vector<std::size_t> out;
    out.reserve(a.size() + b.size());
    std::set_difference(a.begin(), a.end(),
                        b.begin(), b.end(),
                        std::back_inserter(out));

    return out;
}

} // anonymous namespace

//------------------------------------------------------------------------------
ship_editor::ship_editor()
    : _view{}
    , _cursor{}
    , _shape_index{}
    , _snap_to_grid(true)
    , _grid_size(.5f)
{
    _view.viewport.maxs() = application::singleton()->window()->size();
    _view.size = {64.f, 64.f * float(_view.viewport.maxs().y) / float(_view.viewport.maxs().x)};

    _palette.shapes.push_back( // 1x1 square
        shape{{vec2{-.5f, -.5f}, vec2{-.5f, .5f}, vec2{.5f, .5f}, vec2{.5f, -.5f}}});
    _palette.shapes.push_back( // 1x2 square
        shape{{vec2{-1.f, -.5f}, vec2{-1.f, .5f}, vec2{1.f, .5f}, vec2{1.f, -.5f}}});
    _palette.shapes.push_back( // 2x2 square
        shape{{vec2{-1.f, -1.f}, vec2{-1.f, 1.f}, vec2{1.f, 1.f}, vec2{1.f, -1.f}}});
    _palette.shapes.push_back( // 2-2-2 triangle
        shape{{vec2{-1.f, -math::sqrt3<float> / 3.f}, vec2{1.f, -math::sqrt3<float> / 3.f}, vec2{0.f, math::sqrt3<float> * 2.f / 3.f}}});
    _palette.shapes.push_back( // 1-1-1-1-1-1 hexagon
        shape{{vec2{-1.f, 0.f}, vec2{-.5f, -math::sqrt3<float> / 2.f}, vec2{.5f, -math::sqrt3<float> / 2.f}, vec2{1.f, 0.f}, vec2{.5f, math::sqrt3<float> / 2.f}, vec2{-.5f, math::sqrt3<float> / 2.f}}});
    _palette.shapes.push_back( // 1-1-1-1-1-1-1-1 octagon
        shape{{
            vec2{-math::sqrt2<float> / 2.f - .5f, .5f}, vec2{-math::sqrt2<float> / 2.f - .5f, -.5f},
            vec2{-.5f, -math::sqrt2<float> / 2.f - .5f}, vec2{.5f, -math::sqrt2<float> / 2.f - .5f},
            vec2{math::sqrt2<float> / 2.f + .5f, -.5f}, vec2{math::sqrt2<float> / 2.f + .5f, .5f},
            vec2{.5f, math::sqrt2<float> / 2.f + .5f}, vec2{-.5f, math::sqrt2<float> / 2.f + .5f}}});
    // 2-2-1 triangle
    // 2-2-x wedge
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
vec2 ship_editor::snap_position(vec2 p) const
{
    vec2 s = vec2_zero;
    bool snap = false;

    {
        vec2 g = p;
        if (snap_to_grid(g)) {
            if (!snap || length_sqr(g - p) < length_sqr(s - p)) {
                s = g;
                snap = true;
            }
        }
    }

    {
        vec2 e = p;
        if (snap_to_edge(e)) {
            if (!snap || length_sqr(e - p) < length_sqr(s - p)) {
                s = e;
                snap = true;
            }
        }
    }

    {
        vec2 v = p;
        if (snap_to_vertex(v)) {
            if (!snap || length_sqr(v - p) < length_sqr(s - p)) {
                s = v;
                snap = true;
            }
        }
    }

    return snap ? s : p;
}

//------------------------------------------------------------------------------
bool ship_editor::snap_to_grid(vec2& p) const
{
    if (!_snap_to_grid) {
        return false;
    }
    p = vec2(std::round(p.x / _grid_size) * _grid_size,
             std::round(p.y / _grid_size) * _grid_size);
    return true;
}

//------------------------------------------------------------------------------
bool ship_editor::snap_to_edge(vec2& p) const
{
    if (!_snap_to_edge || _shape_index == shape::invalid_index) {
        return false;
    }

    auto const& s1 = _palette.shapes[_shape_index];
    mat3 t1 = mat3::transform(p, 0.f);

    for (size_t si = 0; si < _shape_instances.size(); ++si) {
        auto const& s2 = _palette.shapes[_shape_instances[si].index];
        mat3 t2 = mat3::transform(_shape_instances[si].position,
                                  _shape_instances[si].rotation);

        for (size_t ii = 0; ii < s1.points.size(); ++ii) {
            vec2 p1 = s1.points[ii] * t1;
            vec2 p2 = s1.points[(ii + 1) % s1.points.size()] * t1;
            vec2 dir1 = normalize(p2 - p1);
            vec2 perp = dir1.cross(1.f);

            for (size_t jj = 0; jj < s2.points.size(); ++jj) {
                vec2 p3 = s2.points[jj] * t2;
                vec2 p4 = s2.points[(jj + 1) % s2.points.size()] * t2;
                vec2 dir2 = normalize(p4 - p3);

                float sint = cross(dir1, dir2);
                if (abs(sint) > 1e-3f) {
                    continue;
                }

                float d = dot(p3 - p1, perp);
                if (d * d < .1f) {
                    p += perp * d;
                    return true;
                }
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------------
bool ship_editor::snap_to_vertex(vec2& p) const
{
    if (!_snap_to_vertex || _shape_index == shape::invalid_index) {
        return false;
    }

    auto const& s1 = _palette.shapes[_shape_index];
    mat3 t1 = mat3::transform(p, 0.f);

    for (size_t si = 0; si < _shape_instances.size(); ++si) {
        auto const& s2 = _palette.shapes[_shape_instances[si].index];
        mat3 t2 = mat3::transform(_shape_instances[si].position,
                                  _shape_instances[si].rotation);

        for (size_t ii = 0; ii < s1.points.size(); ++ii) {
            vec2 p1 = s1.points[ii] * t1;

            for (size_t jj = 0; jj < s2.points.size(); ++jj) {
                vec2 p2 = s2.points[jj] * t2;

                if (length_sqr(p2 - p1) < .1f) {
                    p += (p2 - p1);
                    return true;
                }
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------------
void ship_editor::draw(render::system* renderer, time_value time) const
{
    renderer->set_view(_view);

    draw_palette(renderer, time);

    vec2 grid_offset = cursor_to_world();
    if (snap_to_grid(grid_offset)) {
        for (int ii = 0; ii < 3; ++ii) {
            vec2 a = vec2(0, _grid_size * (ii - 1));
            vec2 b = vec2(_grid_size * (ii - 1), 0);
            renderer->draw_line(a + grid_offset, a + grid_offset + vec2(-_grid_size * 6.f, 0), color4(0, .5f, 1.f, 1), color4(0, .5f, 1.f, 0));
            renderer->draw_line(a + grid_offset, a + grid_offset + vec2(_grid_size * 6.f, 0), color4(0, .5f, 1.f, 1), color4(0, .5f, 1.f, 0));

            renderer->draw_line(b + grid_offset, b + grid_offset + vec2(0, -_grid_size * 6.f), color4(0, .5f, 1.f, 1), color4(0, .5f, 1.f, 0));
            renderer->draw_line(b + grid_offset, b + grid_offset + vec2(0, _grid_size * 6.f), color4(0, .5f, 1.f, 1), color4(0, .5f, 1.f, 0));
        }
    }

    if (_shape_index != shape::invalid_index) {
        vec2 offset = snap_position(cursor_to_world());
        auto const& shape = _palette.shapes[_shape_index];
        renderer->draw_line(
            shape.points.back() + offset,
            shape.points.front() + offset,
            color4(0, 1, 1, 1),
            color4(0, 1, 1, 1));
        for (std::size_t jj = 1; jj < shape.points.size(); ++jj) {
            renderer->draw_line(
                shape.points[jj - 1] + offset,
                shape.points[jj] + offset,
                color4(0, 1, 1, 1),
                color4(0, 1, 1, 1));
        }
    }

    for (std::size_t ii = 0; ii < _shape_instances.size(); ++ii) {
        vec2 offset = _shape_instances[ii].position;
        auto const& shape = _palette.shapes[_shape_instances[ii].index];
        renderer->draw_line(
            shape.points.back() + offset,
            shape.points.front() + offset,
            color4(1, 1, 1, .7f),
            color4(1, 1, 1, .7f));
        for (std::size_t jj = 1; jj < shape.points.size(); ++jj) {
            renderer->draw_line(
                shape.points[jj - 1] + offset,
                shape.points[jj] + offset,
                color4(1, 1, 1, .7f),
                color4(1, 1, 1, .7f));
        }
    }
}

//------------------------------------------------------------------------------
void ship_editor::draw_palette(render::system* renderer, time_value time) const
{
    (void)time;

    vec2 palette_offset = _view.origin + vec2(0, -_view.size.y * .4f);
    
    for (std::size_t ii = 0; ii < _palette.shapes.size(); ++ii) {
        auto const& shape = _palette.shapes[ii];
        vec2 offset = vec2((float(ii) - float(_palette.shapes.size()) * .5f) * 4, 0) + palette_offset;
        color4 color = (ii == _shape_index) ? color4(0, 1, 1, 1) : color4(0, 1, 1, .4f);
        renderer->draw_line(
            shape.points.back() + offset,
            shape.points.front() + offset,
            color,
            color);
        for (std::size_t jj = 1; jj < shape.points.size(); ++jj) {
            renderer->draw_line(
                shape.points[jj - 1] + offset,
                shape.points[jj] + offset,
                color,
                color);
        }
    }
}

//------------------------------------------------------------------------------
bool ship_editor::key_event(int key, bool down)
{
    if (!down) {
        switch (key) {
            case K_MOUSE1:
                on_click();
                break;

            case K_CTRL:
                break;

            case K_SHIFT:
                break;

            default:
                break;
        }

        return false;
    }

    switch (key) {
        case K_MOUSE1:
            break;

        case K_MOUSE2:
            break;

        case '-':
            _view.size *= 1.1f;
            break;

        case '=':
            _view.size /= 1.1f;
            break;

        case '[':
            _shape_index = (_shape_index + _palette.shapes.size() - 1) % _palette.shapes.size();
            break;

        case ']':
            _shape_index = (_shape_index + 1) % _palette.shapes.size();
            break;

        case 'g':
            _snap_to_grid = !_snap_to_grid;
            break;
    }

    return false;
}

//------------------------------------------------------------------------------
void ship_editor::cursor_event(vec2 position)
{
    _cursor = position;
}

//------------------------------------------------------------------------------
void ship_editor::on_click()
{
    _shape_instances.push_back({
        _shape_index,
        snap_position(cursor_to_world()),
        0.f });
}

} // namespace editor
