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

//------------------------------------------------------------------------------
fire_director::fire_director(game::ship* owner, gun_design const* gun)
    : subsystem(owner)
    , _gun(gun)
    , _bearing(0)
    , _elevation(0)
    , _time_of_flight(time_delta::zero)
    , _table{}
    , _table_size(0)
    , _is_valid(false)
{
    populate_table();
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
    _bearing = 0;
    _elevation = 0;
    _time_of_flight = time_delta::zero;
    _is_valid = false;

    update_solution();
}

//------------------------------------------------------------------------------
void fire_director::get_solution(double& bearing, double& elevation) const
{
    // Return the best available solution even if invalid to allow 'pre-aiming'
    bearing = _bearing;
    elevation = _elevation;
}

//------------------------------------------------------------------------------
void fire_director::update_solution()
{
    range r;

    if (!_target) {
        return;
    }

    // Calculate range and bearing
    vec3 dv = _target->get_linear_velocity() - _owner->get_linear_velocity();
    vec3 dr = _target->get_position() - _owner->get_position() + dv * _time_of_flight.to_seconds();
    vec3 dir = dr * _owner->get_rotation().inverse();
    double dist = dir.normalize_length();
    _bearing = std::atan2(dir.y, dir.x);

    // Calculate elevation and time of flight
    _is_valid = interpolate_range(dist, r);
    _elevation = r.elevation;
    _time_of_flight = r.time_of_flight;
}

//------------------------------------------------------------------------------
bool fire_director::interpolate_range(double d, range& r) const
{
    std::size_t ii = 0, jj = _table_size - 1;
    // Check boundary conditions, return best possible solution
    if (d < _table[ii].range) {
        r = _table[ii];
        return false;
    } else if (d > _table[jj].range) {
        r = _table[jj];
        return false;
    }

    do {
        std::size_t mid = (ii + jj) / 2;
        if (d >= _table[mid].range) {
            ii = mid;
        } else {
            jj = mid;
        }
    } while (ii + 1 < jj);

    // linear interpolation
    double t = (d - _table[ii].range) / (_table[jj].range - _table[ii].range);
    r.elevation = _table[ii].elevation + (_table[jj].elevation - _table[ii].elevation) * t;
    r.range = d;
    r.time_of_flight = _table[ii].time_of_flight + (_table[jj].time_of_flight - _table[ii].time_of_flight) * t;
    return true;
}

//------------------------------------------------------------------------------
void fire_director::populate_table()
{
    _table_size = max_table;

    for (std::size_t ii = 0; ii < max_table; ++ii) {
        double elevation = ii * ((max_elevation - min_elevation) * (1.0 / double(max_table))) - min_elevation;

        vec3 start = globe::lonlat_to_surface(vec2(0,0)) + vec3(1,0,0);
        vec3 pos = start;
        vec3 vel = vec3(sin(elevation), cos(elevation), 0) * _gun->shell_velocity;

        _table[ii].elevation = elevation;
        _table[ii].time_of_flight = ballistics::simulate(
            pos,
            vel,
            ballistics::curve::G1,
            _gun->shell_coefficient,
            FRAMETIME);
        _table[ii].range = length(pos - start);

        if (ii > 0 && _table[ii].range < _table[ii - 1].range) {
            _table_size = ii;
            break;
        }
    }
}

} // namespace game
