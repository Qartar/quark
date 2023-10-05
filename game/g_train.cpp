// g_train.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_train.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type train::_type(object::_type);

//------------------------------------------------------------------------------
train::train()
    : _next_station(SIZE_MAX)
#if 0
    , _current_distance(0)
    , _current_speed(0)
    , _num_cars(0)
#else
    , _current_distance(16)
    , _current_speed(30)
    , _num_cars(16)
#endif
{
    _path.push_back(0);
    _path.push_back(2);
    _path.push_back(4);
    _path.push_back(6);
    _path.push_back(8);
    _path.push_back(10);

    _path.push_back(0);
    _path.push_back(12);
    _path.push_back(14);
    _path.push_back(16);
    _path.push_back(18);
    _path.push_back(20);
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
    (void)renderer;
    (void)time;

    if (!_path.size()) {
        return;
    }

    float dist = _current_distance + _current_speed * (time - get_world()->frametime()).to_seconds();
    for (std::size_t ii = 0; ii < _path.size(); ++ii) {
        clothoid::segment seg = get_world()->rail_network().get_segment(_path[ii]);

        if (dist < seg.length()) {
            draw_locomotive(renderer, seg, dist);
            break;
        }

        dist -= seg.length();
    }

    for (int ii = 0; ii < _num_cars; ++ii) {
        dist = _current_distance - car_offset(ii) + _current_speed * (time - get_world()->frametime()).to_seconds();

        for (std::size_t jj = 0; jj < _path.size(); ++jj) {
            clothoid::segment seg = get_world()->rail_network().get_segment(_path[jj]);

            if (dist < seg.length()) {
                draw_car(renderer, seg, dist);
                break;
            }

            dist -= seg.length();
        }
    }
}

//------------------------------------------------------------------------------
void train::draw_locomotive(render::system* renderer, clothoid::segment segment, float s) const
{
    vec2 pos = segment.evaluate(s);
    vec2 dir = segment.evaluate_tangent(s);
    float k = segment.evaluate_curvature(s);
    // offset laterally to align the trucks onto the rail instead of the center
    if (k) {
        constexpr float truck_offset = 9.6f;
        pos -= (1.f - cos(k * truck_offset)) * dir.cross(1.f / k);
    }
    mat3 tx = mat3(dir.x, dir.y, 0,
                  -dir.y, dir.x, 0,
                   pos.x, pos.y, 1);

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

    renderer->draw_line(pts[0], pts[1], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[1], pts[3], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[3], pts[5], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[5], pts[4], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[4], pts[2], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[2], pts[0], color4(1,1,1,1), color4(1,1,1,1));

#if 0
    float target = target_speed();
    renderer->draw_string(
        va("%.0f / %.0f", _current_speed, target),
        pos,
        _current_speed > target + .5f ? color4(1,0,0,1) : color4(1,1,1,1));
#elif 0
    renderer->draw_string(va("%zu", _prev.size()), pos, color4(1,1,1,1));
#endif
}

//------------------------------------------------------------------------------
void train::draw_car(render::system* renderer, clothoid::segment segment, float s) const
{
    vec2 pos = segment.evaluate(s);
    vec2 dir = segment.evaluate_tangent(s);
    float k = segment.evaluate_curvature(s);
    // offset laterally to align the trucks onto the rail instead of the center
    if (k) {
        constexpr float truck_offset = 5.6f;
        pos -= (1.f - cos(k * truck_offset)) * dir.cross(1.f / k);
    }
    mat3 tx = mat3(dir.x, dir.y, 0,
                  -dir.y, dir.x, 0,
                   pos.x, pos.y, 1);

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

    renderer->draw_line(pts[0], pts[1], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[1], pts[3], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[3], pts[2], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[2], pts[0], color4(1,1,1,1), color4(1,1,1,1));
}

//------------------------------------------------------------------------------
void train::think()
{
    if (!_path.size()) {
        return;
    }

    clothoid::segment seg = get_world()->rail_network().get_segment(_path[0]);
    float train_length = car_offset(_num_cars - 1);

    _current_distance += _current_speed * FRAMETIME.to_seconds();
    if (_current_distance - train_length > seg.length()) {
        _current_distance -= seg.length();
        if (_path.size() > 1) {
            _path.push_back(_path[0]); // TEMP: loop
            _path.erase(_path.begin());
        }
    }

    float s = target_speed();
    if (_current_speed > s) {
        _current_speed = max(s, _current_speed - max_deceleration * FRAMETIME.to_seconds());
    } else if (_current_speed < s) {
        _current_speed = min(s, _current_speed + max_acceleration * FRAMETIME.to_seconds());
    }
}

//------------------------------------------------------------------------------
vec2 train::get_position(time_value time) const
{
    (void)time;
    return vec2_zero;
}

//------------------------------------------------------------------------------
float train::get_rotation(time_value time) const
{
    (void)time;
    return 0.f;
}

//------------------------------------------------------------------------------
mat3 train::get_transform(time_value time) const
{
    (void)time;
    return mat3_identity;
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
    constexpr float locomotive_length = 24.f;
    constexpr float car_length = 16.f;
    constexpr float coupling_length = 1.f;

    return .5f * (locomotive_length + car_length)
        + coupling_length
        + index * (car_length + coupling_length);
}

} // namespace game
