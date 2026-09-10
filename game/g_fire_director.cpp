// g_fire_director.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_fire_director.h"
#include "g_ballistics.h"
#include "g_globe.h"
#include "g_ship.h"
#include "design/g_gun_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type fire_director::_type(subsystem::_type);

config::boolean fire_director::_show_correction("g_show_correction", false, 0, "Show fire director correction debug visualization");

//------------------------------------------------------------------------------
fire_director::fire_director(game::ship* owner, gun_design const* gun)
    : subsystem(owner)
    , _gun(gun)
    , _range(0)
    , _bearing(0)
    , _elevation(0)
    , _time_of_flight(time_delta::zero)
    , _is_valid(false)
    , _corrections{}
    , _num_corrections(0)
{
}

//------------------------------------------------------------------------------
void fire_director::draw(render::system* renderer, time_value time) const
{
    if (_show_correction && _num_corrections) {
        vec3 pos = _target->get_position(time);
        float radius = 20.f * _gun->caliber;
        for (std::size_t ii = 0, sz = min(_num_corrections, max_corrections); ii < sz; ++ii) {
            std::size_t idx = (_num_corrections - ii - 1) % max_corrections;
            float alpha = square((max_corrections - ii) * (1.f / float(max_corrections)));
            vec3 v0 = pos + _corrections[idx].total;
            renderer->draw_arc((v0 * renderer->view().transform).to_vec2(), radius, 0, 0, math::twopi, color4(1,1,0,alpha));
            vec3 v1 = pos - _corrections[idx].offset;
            renderer->draw_arc((v1 * renderer->view().transform).to_vec2(), radius, 0, 0, math::twopi, color4(1,0,0,alpha));
        }
    }
}

//------------------------------------------------------------------------------
void fire_director::think()
{
    update_solution();
}

//------------------------------------------------------------------------------
void fire_director::set_target(handle<ship const> target)
{
    _target = target;
    _range = 0;
    _bearing = 0;
    _elevation = 0;
    _time_of_flight = time_delta::zero;
    _is_valid = false;

    _num_corrections = 0;

    update_solution();
}

//------------------------------------------------------------------------------
void fire_director::get_solution(double& range, double& bearing, double& elevation) const
{
    // Return the best available solution even if invalid to allow 'pre-aiming'
    range = _range;
    bearing = _bearing;
    elevation = _elevation;
}

//------------------------------------------------------------------------------
bool fire_director::splash_observation(handle<ship const> target, float shell_size, vec3 splash_origin)
{
    if (target == _target && shell_size == _gun->caliber) {
        vec3 offset = target->get_position() - splash_origin;
        time_value time = get_world()->frametime();

        // Apply offset from most recent relevant correction
        for (std::size_t ii = 0, sz = min(_num_corrections, max_corrections); ii < sz; ++ii) {
            std::size_t idx = (_num_corrections - ii - 1) % max_corrections;
            if (time > _corrections[idx].time + _time_of_flight) {
                offset += _corrections[idx].total;
                break;
            }
        }

        // Update total correction
        std::size_t idx = _num_corrections % max_corrections;
        vec3 total = offset;
        if (_num_corrections >= max_corrections) {
            total = _corrections[(_num_corrections - 1) % max_corrections].total;
            total -= _corrections[idx].offset / double(max_corrections);
            total += offset / double(max_corrections);
        } else if (_num_corrections) {
            total = _corrections[_num_corrections - 1].total * double(_num_corrections) + offset;
            total /= double(_num_corrections + 1);
        }

        _corrections[idx].time = time;
        _corrections[idx].offset = offset;
        _corrections[idx].total = total;
        ++_num_corrections;

        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
void fire_director::update_solution()
{
    gun_design::range_data r;

    if (!_target) {
        return;
    }

    // Calculate range and bearing
    vec3 dv = _target->get_linear_velocity() - _owner->get_linear_velocity();
    vec3 dr = _target->get_position() - _owner->get_position() + dv * _time_of_flight.to_seconds();
    if (_num_corrections) {
        dr += _corrections[(_num_corrections - 1) % max_corrections].total;
    }
    vec3 dir = dr * _owner->get_rotation().inverse();
    _range = dir.normalize_length();
    _bearing = std::atan2(dir.y, dir.x);

    // Calculate elevation and time of flight
    _is_valid = _gun->interpolate_range(_range, r);
    _elevation = r.elevation;
    _time_of_flight = r.time;
}

} // namespace game
