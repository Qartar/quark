// g_train.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_train.h"
#include "g_rail_station.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type train::_type(object::_type);

//------------------------------------------------------------------------------
train::train(int num_cars)
    : _next_station(SIZE_MAX)
    , _current_distance(0)
    , _current_speed(0)
    , _current_acceleration(0)
    , _target_distance(0)
    , _num_cars(num_cars)
    , _wait_time(time_value::zero)
    , _state(state::moving)
{
}

//------------------------------------------------------------------------------
train::~train()
{
}

//------------------------------------------------------------------------------
void train::spawn()
{
}

//------------------------------------------------------------------------------
void train::draw(render::system* renderer, time_value time) const
{
    if (!_path.size()) {
        return;
    }

    float delta_time = (time - get_world()->frametime()).to_seconds();
    float distance = _current_distance
        + _current_speed * delta_time
        + .5f * _current_acceleration * square(delta_time);

    draw(renderer, distance, color4(1,1,1,1));
}

//------------------------------------------------------------------------------
void train::draw(render::system* renderer, float distance, color4 color) const
{
    int num_trucks = _num_cars * 2 + 2;
    vec2* truck_position = (vec2*)alloca(sizeof(vec2) * num_trucks);

    // evaluate truck positions
    float path_length = 0.f;
    for (std::size_t ii = 0, jj = 0; ii < _path.size() && jj < num_trucks; ++ii) {
        clothoid::segment seg = get_world()->rail_network().get_segment(_path[ii]);
        for (; jj < num_trucks; ++jj) {
            float s = distance - this->truck_offset(int(num_trucks - jj - 1)) - path_length;
            if (s > seg.length()) {
                break;
            }
            truck_position[num_trucks - jj - 1] = seg.evaluate(s);
            // draw truck positions
            {
                vec2 p = truck_position[num_trucks - jj - 1];
                vec2 t = seg.evaluate_tangent(s);
                renderer->draw_line(p - t.cross(2), p + t.cross(2), color, color);
            }
        }
        path_length += seg.length();
    }

    // draw couplers
    for (int ii = 0; ii < _num_cars; ++ii) {
        vec2 pos = .5f * (truck_position[ii * 2 + 1] + truck_position[ii * 2 + 2]);
        vec2 dir = (truck_position[ii * 2 + 1] - truck_position[ii * 2 + 2]).normalize();

        mat3 tx = mat3(dir.x, dir.y, 0,
            -dir.y, dir.x, 0,
            pos.x, pos.y, 1);

        draw_coupler(renderer, tx, color);
    }

    // draw locomotive
    {
        vec2 pos = .5f * (truck_position[0] + truck_position[1]);
        vec2 dir = (truck_position[0] - truck_position[1]).normalize();

        mat3 tx = mat3(dir.x, dir.y, 0,
            -dir.y, dir.x, 0,
            pos.x, pos.y, 1);

        draw_locomotive(renderer, tx, color);
    }

    // draw cars
    for (int ii = 0; ii < _num_cars; ++ii) {
        vec2 pos = .5f * (truck_position[ii * 2 + 2] + truck_position[ii * 2 + 3]);
        vec2 dir = (truck_position[ii * 2 + 2] - truck_position[ii * 2 + 3]).normalize();

        mat3 tx = mat3(dir.x, dir.y, 0,
            -dir.y, dir.x, 0,
            pos.x, pos.y, 1);

        draw_car(renderer, tx, color);
    }
}

//------------------------------------------------------------------------------
void train::draw_locomotive(render::system* renderer, mat3 tx, color4 color) const
{
    const vec2 pts[] = {
        vec2(-12, 1.6f) * tx, vec2(-12,-1.6f) * tx,
        vec2(11.3f, 1.6f) * tx, vec2(11.3f, -1.6f) * tx,
        vec2(12, .9f) * tx, vec2(12, -.9f) * tx,
    };

    const color4 clr[] = {
        color4(0,0,0,1), color4(0,0,0,1),
        color4(0,0,0,1), color4(0,0,0,1),
        color4(0,0,0,1), color4(0,0,0,1),
    };

    const int idx[] = {
        0, 1, 3, 0, 3, 2,
        2, 3, 5, 2, 5, 4,
    };

    renderer->draw_triangles(pts, clr, idx, countof(idx));

    renderer->draw_line(pts[0], pts[1], color, color);
    renderer->draw_line(pts[1], pts[3], color, color);
    renderer->draw_line(pts[3], pts[5], color, color);
    renderer->draw_line(pts[5], pts[4], color, color);
    renderer->draw_line(pts[4], pts[2], color, color);
    renderer->draw_line(pts[2], pts[0], color, color);

#if 0
    float target = target_speed();
    renderer->draw_string(
        va("%.0f / %.0f", _current_speed, target),
        pos,
        _current_speed > target + .5f ? color4(1,0,0,1) : color4(1,1,1,1));
#elif 0
    renderer->draw_string(va("%zu", _prev.size()), pos, color4(1,1,1,1));
#elif 0
    renderer->draw_string(
        va("%s %.0f", _schedule[_next_station]->name().c_str(), _target_distance - _current_distance),
        pos,
        color4(1,1,1,1));
#endif
}

//------------------------------------------------------------------------------
void train::draw_car(render::system* renderer, mat3 tx, color4 color) const
{
    vec2 pts[4] = {
        vec2(-8, 1.6f) * tx,
        vec2(-8, -1.6f) * tx,
        vec2(8, 1.6f) * tx,
        vec2(8, -1.6f) * tx,
    };

    const color4 clr[4] = {
        color4(0,0,0,1),
        color4(0,0,0,1),
        color4(0,0,0,1),
        color4(0,0,0,1),
    };

    const int idx[6] = {
        0, 1, 3, 0, 3, 2,
    };

    renderer->draw_triangles(pts, clr, idx, countof(idx));

    renderer->draw_line(pts[0], pts[1], color, color);
    renderer->draw_line(pts[1], pts[3], color, color);
    renderer->draw_line(pts[3], pts[2], color, color);
    renderer->draw_line(pts[2], pts[0], color, color);
}

//------------------------------------------------------------------------------
void train::draw_coupler(render::system* renderer, mat3 tx, color4 color) const
{
    vec2 pts[4] = {
        vec2(-1, .2f) * tx,
        vec2(-1, -.2f) * tx,
        vec2(1, .2f) * tx,
        vec2(1, -.2f) * tx,
    };

    const color4 clr[4] = {
        color4(0,0,0,1),
        color4(0,0,0,1),
        color4(0,0,0,1),
        color4(0,0,0,1),
    };

    const int idx[6] = {
        0, 1, 3, 0, 3, 2,
    };

    renderer->draw_triangles(pts, clr, idx, countof(idx));

    renderer->draw_line(pts[0], pts[1], color, color);
    renderer->draw_line(pts[1], pts[3], color, color);
    renderer->draw_line(pts[3], pts[2], color, color);
    renderer->draw_line(pts[2], pts[0], color, color);
}

//------------------------------------------------------------------------------
void train::think()
{
    if (!_path.size()) {
        return;
    }

    clothoid::segment seg = get_world()->rail_network().get_segment(_path[0]);

    _current_distance += _current_speed * FRAMETIME.to_seconds()
        + .5f * _current_acceleration * square(FRAMETIME.to_seconds());
    _current_speed = max(0.f, _current_speed + _current_acceleration * FRAMETIME.to_seconds());

    if (_current_distance - length() > seg.length()) {
        _current_distance -= seg.length();
        _target_distance -= seg.length();
        _path.erase(_path.begin());
    }

    if (_state == state::moving) {
        if (_current_distance >= _target_distance - 1e-3f) {
            _current_distance = _target_distance;
            _state = state::waiting;
            _wait_time = get_world()->frametime() + time_delta::from_seconds(20);
        }
    }

    if (_state == state::waiting) {
        if (_wait_time > get_world()->frametime()) {
            _current_acceleration = clamp(-_current_speed / FRAMETIME.to_seconds(), -max_deceleration, 0.f);
            return;
        } else {
            _state = state::moving;
            next_station();
        }
    }

    float target_distance = _target_distance;
    float target_acceleration = (target_speed() - _current_speed) / FRAMETIME.to_seconds();

    // stopping distance at current speed
    float d = .5f * square(_current_speed) / max_deceleration;
    // stopping distance assuming maximum acceleration this frame
    float d2 = .5f * square(_current_speed + max_acceleration * FRAMETIME.to_seconds()) / max_deceleration
        + _current_speed * FRAMETIME.to_seconds() + .5f * max_acceleration * square(FRAMETIME.to_seconds());

    // check for potential collisions along path
    float collision_distance;
    if (check_collisions(collision_distance)) {
        if (collision_distance - _current_distance < d2) {
            target_distance = min(target_distance, collision_distance);
        }
    }

    if (target_distance - _current_distance < d) {
        target_acceleration = -max_deceleration;
    } else if (target_distance - _current_distance < d2) {
        // solve for acceleration this frame that moves the train precisely to
        // the stopping distance next frame (at maximum deceleration)
        float a = .5f * square(FRAMETIME.to_seconds());
        float b = _current_speed * FRAMETIME.to_seconds()
            + .5f * max_deceleration * square(FRAMETIME.to_seconds());
        float c = .5f * square(_current_speed)
            + max_deceleration * _current_speed * FRAMETIME.to_seconds()
            - (target_distance - _current_distance) * max_deceleration;
        float det = b * b - 4.f * a * c;
        // always use the second (or singular) solution if it exists
        if (det >= 0.f) {
            float q = -.5f * (b + std::copysign(std::sqrt(det), b));
            target_acceleration = min(c / q, target_acceleration);
        }
    }

    _current_acceleration = clamp(target_acceleration, -max_deceleration, max_acceleration);
}

//------------------------------------------------------------------------------
vec2 train::get_position(time_value time) const
{
    float delta_time = (time - get_world()->frametime()).to_seconds();
    float distance = _current_distance
        + _current_speed * delta_time
        + .5f * _current_acceleration * square(delta_time);

    for (std::size_t ii = 0; ii < _path.size(); ++ii) {
        clothoid::segment seg = get_world()->rail_network().get_segment(_path[ii]);

        if (distance <= seg.length() || ii == _path.size() - 1) {
            return seg.evaluate(distance);
        }

        distance -= seg.length();
    }

    return vec2_zero;
}

//------------------------------------------------------------------------------
float train::get_rotation(time_value time) const
{
    float delta_time = (time - get_world()->frametime()).to_seconds();
    float distance = _current_distance
        + _current_speed * delta_time
        + .5f * _current_acceleration * square(delta_time);

    for (std::size_t ii = 0; ii < _path.size(); ++ii) {
        clothoid::segment seg = get_world()->rail_network().get_segment(_path[ii]);

        if (distance <= seg.length() || ii == _path.size() - 1) {
            vec2 dir = seg.evaluate_tangent(distance);
            return atan2f(dir.y, dir.x);
        }

        distance -= seg.length();
    }

    return 0.f;
}

//------------------------------------------------------------------------------
mat3 train::get_transform(time_value time) const
{
    (void)time;
    return mat3_identity;
}

//------------------------------------------------------------------------------
void train::set_schedule(std::vector<handle<rail_station>> const& schedule)
{
    _schedule = schedule;
    _next_station = SIZE_MAX;
    next_station();
}

//------------------------------------------------------------------------------
void train::next_station()
{
    if (!_schedule.size()) {
        return;
    }

    _next_station = (_next_station + 1) % _schedule.size();
    if (!_path.size()) {
        _path.push_back(_schedule[_next_station]->edge());
        _current_distance = _schedule[_next_station]->dist();
        _target_distance = _current_distance;
        return;
    }

    float path_distance = 0.f;
    float stopping_distance = .5f * square(_current_speed) / max_deceleration;

    rail_position start{};
    rail_position goal = rail_position::from_edge(
        _schedule[_next_station]->edge(),
        _schedule[_next_station]->dist());

    std::size_t prev_size = _path.size();
    for (std::size_t ii = 0; ii < _path.size(); ++ii) {
        clothoid::segment s = get_world()->rail_network().get_segment(_path[ii]);
        if (path_distance + s.length() > _current_distance + stopping_distance) {
            prev_size = ii + 1;
            start = rail_position::from_edge(
                _path[ii],
                _current_distance + stopping_distance - path_distance);
            path_distance += s.length();
            break;
        }
        path_distance += s.length();
    }

    edge_index path[1024];
    std::size_t path_size = get_world()->rail_network().find_path(
        start,
        goal,
        path,
        countof(path));

    _path.resize(prev_size + path_size);
    for (std::size_t ii = 0; ii < path_size; ++ii) {
        if (path[ii] == goal.edge) {
            _target_distance = path_distance + goal.dist;
        } else {
            clothoid::segment s = get_world()->rail_network().get_segment(path[ii]);
            path_distance += s.length();
        }
        _path[prev_size + ii] = path[ii];
    }
}

//------------------------------------------------------------------------------
float train::target_speed() const
{
    constexpr float stopping_distance = .5f * square(max_speed) / max_deceleration;
    float tail_distance = _current_distance - car_offset(_num_cars - 1);
    float segment_distance = 0.f;

    float maximum_speed = max_speed;
    for (std::size_t ii = 0, sz = _path.size(); ii < sz; ++ii) {
        clothoid::segment segment = get_world()->rail_network().get_segment(_path[ii]);

        switch (segment.type()) {
            case clothoid::segment_type::line:
                break;

            case clothoid::segment_type::arc: {
                float s = max(0.f, segment_distance - _current_distance);
                float k = abs(segment.initial_curvature());
                float vsqr = max_lateral_acceleration / k;
                maximum_speed = min(maximum_speed, sqrt(vsqr + 2.f * s * max_deceleration));
                break;
            }

            case clothoid::segment_type::transition: {
                if (abs(segment.initial_curvature()) > abs(segment.final_curvature())) {
                    if (tail_distance > segment_distance) {
                        float s = tail_distance - segment_distance;
                        float k = abs(segment.evaluate_curvature(s));
                        float vsqr = max_lateral_acceleration / k;
                        maximum_speed = min(maximum_speed, sqrt(vsqr));
                    } else {
                        float d = max(0.f, segment_distance - _current_distance);
                        float k = abs(segment.initial_curvature());
                        float vsqr = max_lateral_acceleration / k;
                        maximum_speed = min(maximum_speed, sqrt(vsqr + 2.f * d * max_deceleration));
                    }
                } else if (_current_distance < segment_distance + segment.length()) {
                    // solve d/ds maximum_speed = 0 for s
                    float k0 = abs(segment.initial_curvature());
                    float k1 = abs(segment.final_curvature());
                    float a = (k1 - k0) / (max_lateral_acceleration * segment.length());
                    float b = k0 / max_lateral_acceleration;
                    float c = 2.f * max_deceleration;
                    // solve c - a / (ax + b)^2 = 0
                    float s = (sqrt(a) - b * sqrt(c)) / (a * sqrt(c));
                    s = clamp(s, 0.f, segment.length());
                    s = max(s, _current_distance - segment_distance);
                    float d = max(0.f, s + segment_distance - _current_distance);
                    float k = abs(segment.evaluate_curvature(s));
                    float vsqr = max_lateral_acceleration / k;
                    maximum_speed = min(maximum_speed, sqrt(vsqr + 2.f * d * max_deceleration));
                } else {
                    float k = abs(segment.final_curvature());
                    float vsqr = max_lateral_acceleration / k;
                    maximum_speed = min(maximum_speed, sqrt(vsqr));
                }
                break;
            }

            default:
                __assume(false);
        }

        segment_distance += segment.length();
        if (segment_distance > _current_distance + stopping_distance) {
            break;
        }
    }

    return maximum_speed;
}

//------------------------------------------------------------------------------
float train::car_offset(int index) const
{
    return (locomotive_length + coupling_length)
        + index * (car_length + coupling_length);
}

//------------------------------------------------------------------------------
float train::truck_offset(int index) const
{
    if (index == 0) {
        return 2.4f;
    } else if (index == 1) {
        return locomotive_length - 2.4f;
    } else if ((index & 1) == 0) {
        return car_offset(index / 2 - 1) + 2.4f;
    } else {
        return car_offset(index / 2 - 1) + car_length - 2.4f;
    }
}

//------------------------------------------------------------------------------
float train::length() const
{
    return car_offset(_num_cars);
}

//------------------------------------------------------------------------------
bool train::check_collisions(float& collision_distance) const
{
    collision_distance = FLT_MAX;
    // FIXME: cycle through all objects
    for (auto obj : get_world()->objects()) {
        if (obj->is_type<train>() && obj != this) {
            float distance;
            if (check_collision(obj->as_type<train>(), distance)) {
                if (distance > _current_distance - distance_epsilon
                        && distance < collision_distance) {
                    collision_distance = distance;
                }
            }
        }
    }

    return collision_distance < FLT_MAX;
}

//------------------------------------------------------------------------------
bool train::check_collision(train const* other, float& collision_distance) const
{
    edge_index edge;
    float offset, other_offset, length;
    if (!check_path_intersection(other, edge, offset, other_offset, length)) {
        return false;
    }

    float enter_time = (offset - _current_distance) * other->_current_speed;
    float other_enter_time = (other_offset - other->_current_distance) * _current_speed;

    if (enter_time < other_enter_time) {
        return false;
    }

    float clearance = 15.f;
    float other_length = other->length() + clearance;
    float other_tail_time = (other_offset + other_length - other->_current_distance) * _current_speed;

    if (_current_distance - distance_epsilon < offset
            && other->_current_distance > other_offset
            && other->_current_distance - other_length < other_offset) {
        collision_distance = offset;
        return true;
    } else if (enter_time > other_tail_time
            || (_current_distance > offset && other->_current_distance > other_offset)) {
        // current distance relative to path intersection
        float d0 = _current_distance - offset;
        // other distance relative to path intersection
        float d1 = other->_current_distance - other_offset - other_length;

        collision_distance = _current_distance + (d1 - d0) + .5f * square(other->_current_speed) / max_deceleration;
        return true;
    } else {
        collision_distance = offset;
        return true;
    }
}

//------------------------------------------------------------------------------
bool train::check_path_intersection(train const* other, edge_index& edge, float& offset, float& other_offset, float& length) const
{
    auto const& network = get_world()->rail_network();
    const float stopping_distance = .5f * square(_current_speed) / max_deceleration;

    offset = 0.f;
    // iterate over this train's path edges
    for (std::size_t ii = 0, si = _path.size(); ii < si; ++ii) {
        other_offset = 0.f;
        // iterate over other train's path edges until there is an overlap
        for (std::size_t jj = 0, sj = other->_path.size(); jj < sj; ++jj) {
            if (network.start_node(_path[ii]) == network.start_node(other->_path[jj])) {
                edge = _path[ii];
                length = 0.f;
                float clearance = (ii && jj) ? network.get_clearance(_path[ii - 1] ^ 1, other->_path[jj - 1] ^ 1) : 0.f;
                float other_clearance = (ii && jj) ? network.get_clearance(other->_path[jj - 1] ^ 1, _path[ii - 1] ^ 1) : 0.f;
                // continue iterating over other train's path to find the end of overlap
                for (std::size_t kk = 0;; ++kk) {
                    if (ii + kk >= si || jj + kk >= sj
                        || _path[ii + kk] != other->_path[jj + kk]) {
                        break;
                    }
                    length += network.get_segment(_path[ii + kk]).length();
                }
                offset -= max(clearance, other_clearance);
                other_offset -= max(clearance, other_clearance);
                length += max(clearance, other_clearance);
                return true;
            }

            other_offset += network.get_segment(other->_path[jj]).length();
        }

        if (offset - _current_distance > stopping_distance) {
            return false;
        }

        offset += network.get_segment(_path[ii]).length();
    }

    return false;
}

} // namespace game
