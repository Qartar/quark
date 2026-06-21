// g_formation.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_formation.h"
#include "g_navigation.h"
#include "g_ship.h"
#include "design/g_ship_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type formation::_type(object::_type);

//------------------------------------------------------------------------------
formation::formation()
    : _size(0)
    , _spacing(1000)
    , _target_position{}
    , _target_velocity{}
    , _target_speed(DBL_MAX)
    , _maximum_speed(DBL_MAX)
{
}

//------------------------------------------------------------------------------
formation::~formation()
{
}

//------------------------------------------------------------------------------
void formation::draw(render::system* renderer, time_value /*time*/) const
{
    mat4 const tx = renderer->view().transform;
    for (std::size_t ii = 0; ii < _size; ++ii) {
        vec2 v0 = (_target_position[ii] * tx).to_vec2();
        vec2 v1 = v0 + (_target_velocity[ii] * tx).to_vec2();
        renderer->draw_arc(v0, 10.f, 0.f, 0.f, math::twopi, color4(1,1,1,1)); 
        renderer->draw_line(v0, v1, color4(1,1,1,1), color4(1,1,1,1));
    }
}

//------------------------------------------------------------------------------
void formation::think()
{
    if (!_size) {
        return;
    }

    // Update the lead position and velocity directly
    if (auto* f = _objects[0]->cast<formation>()) {
        _target_position[0] = f->target_position(0);
        _target_velocity[0] = f->target_velocity(0);
    } else if (auto* s = _objects[0]->cast<ship>()) {
        _target_position[0] = s->get_position();
        _target_velocity[0] = s->get_linear_velocity();
    }

    // Simple string-pulling for line-ahead formation
    for (std::size_t ii = 1; ii < _size; ++ii) {
        vec3 dir = normalize(_target_position[ii - 1] - _target_position[ii]);
        _target_position[ii] = _target_position[ii - 1] - dir * _spacing;
        _target_velocity[ii] = dir * _target_speed;
    }
}

//------------------------------------------------------------------------------
void formation::set(handle<object> const* objects, std::size_t size)
{
    assert(size <= maximum_size);

    // Remove existing objects
    for (std::size_t ii = 0; ii < _size; ++ii) {
        if (auto* s = _objects[ii]->cast<ship>()) {
            s->navigation()->set_formation(nullptr, 0);
        }
    }

    // Add new objects
    _size = size;
    _maximum_speed = DBL_MAX;
    for (std::size_t ii = 0; ii < _size; ++ii) {
        _objects[ii] = objects[ii];
        if (auto* f = _objects[ii]->cast<formation>()) {
            _maximum_speed = min(_maximum_speed, f->maximum_speed());
            _target_position[ii] = f->target_position(0);
        } else if (auto* s = _objects[ii]->cast<ship>()) {
            _maximum_speed = min<double>(_maximum_speed, s->design()->speed);
            _target_position[ii] = s->get_position();
            s->navigation()->set_formation(this, ii);
        }
    }

    _target_speed = _maximum_speed;
}

//------------------------------------------------------------------------------
void formation::add(handle<object> obj)
{
    assert(_size < maximum_size);

    _objects[_size] = obj;
    if (auto* f = _objects[_size]->cast<formation>()) {
        _maximum_speed = min(_maximum_speed, f->maximum_speed());
        _target_position[_size] = _objects[_size]->cast<formation>()->target_position(0);
    } else if (auto* s = _objects[_size]->cast<ship>()) {
        _maximum_speed = min<double>(_maximum_speed, s->design()->speed);
        _target_position[_size] = s->get_position();
        s->navigation()->set_formation(this, _size);
    }
    _target_speed = min(_target_speed, _maximum_speed);
    ++_size;
}

//------------------------------------------------------------------------------
void formation::remove(handle<object> obj)
{
    for (std::size_t ii = 0; ii < _size; ++ii) {
        if (_objects[ii] != obj) {
            continue;
        }

        if (auto* s = _objects[ii]->cast<ship>()) {
            s->navigation()->set_formation(nullptr, 0);
        }

        --_size;
        for (std::size_t jj = ii; jj < _size; ++jj) {
            _objects[jj] = jj + 1 < maximum_size ? _objects[jj + 1] : nullptr;
            if (_objects[jj]) {
                if (auto* s = _objects[ii]->cast<ship>()) {
                    s->navigation()->set_formation(this, jj);
                }
            } else {
                return;
            }
        }
    }
}

} // namespace game
