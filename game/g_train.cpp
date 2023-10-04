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
    , _current_edge(clothoid::network::invalid_edge)
    , _current_distance(0)
    , _current_speed(0)
    , _num_cars(0)
#else
    , _current_edge(0)
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

    if (_current_edge == clothoid::network::invalid_edge) {
        return;
    }

    clothoid::segment seg = get_world()->rail_network().get_segment(_current_edge);

    float dist = _current_distance + _current_speed * (time - get_world()->frametime()).to_seconds();
    if (dist > seg.length()) {
        if (_path.size() > 1) {
            dist -= seg.length();
            seg = get_world()->rail_network().get_segment(_path[1]);
        }
    }

    vec2 pos = seg.evaluate(dist);
    vec2 dir = seg.evaluate_tangent(dist);

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

    renderer->draw_triangles(
        pts,
        clr,
        idx,
        countof(idx));

    renderer->draw_line(pts[0], pts[1], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[1], pts[3], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[3], pts[5], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[5], pts[4], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[4], pts[2], color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_line(pts[2], pts[0], color4(1,1,1,1), color4(1,1,1,1));
#if 0
    float s = target_speed();
    renderer->draw_string(
        va("%.0f / %.0f", _current_speed, s),
        pos,
        _current_speed > s + .5f ? color4(1,0,0,1) : color4(1,1,1,1));
#elif 0
    renderer->draw_string(va("%zu", _prev.size()), pos, color4(1,1,1,1));
#endif
    for (int ii = 0; ii < _num_cars; ++ii) {
        float car_dist = car_offset(ii);
        float offset = dist;

        seg = get_world()->rail_network().get_segment(_current_edge);
        if (offset < _current_distance) {
            offset += seg.length();
        }

        std::size_t jj = 0;
        while (car_dist > offset && jj < _prev.size()) {
            seg = get_world()->rail_network().get_segment(_prev[_prev.size() - jj - 1]);
            offset += seg.length();
            jj++;
        }

        if (car_dist < offset) {
            pos = seg.evaluate(offset - car_dist);
            dir = seg.evaluate_tangent(offset - car_dist);
            tx = mat3(dir.x, dir.y, 0,
                     -dir.y, dir.x, 0,
                      pos.x, pos.y, 1);

            vec2 car_pts[4] = {
                vec2(-8, 1.6f) * tx,
                vec2(-8, -1.6f) * tx,
                vec2(8, 1.6f) * tx,
                vec2(8, -1.6f) * tx,
            };

            renderer->draw_triangles(
                car_pts,
                clr,
                idx,
                6);

            renderer->draw_line(car_pts[0], car_pts[1], color4(1,1,1,1), color4(1,1,1,1));
            renderer->draw_line(car_pts[1], car_pts[3], color4(1,1,1,1), color4(1,1,1,1));
            renderer->draw_line(car_pts[3], car_pts[2], color4(1,1,1,1), color4(1,1,1,1));
            renderer->draw_line(car_pts[2], car_pts[0], color4(1,1,1,1), color4(1,1,1,1));
        }
    }
}

//------------------------------------------------------------------------------
void train::think()
{
    if (_current_edge == clothoid::network::invalid_edge) {
        return;
    }

    clothoid::segment seg = get_world()->rail_network().get_segment(_current_edge);

    _current_distance += _current_speed * FRAMETIME.to_seconds();
    if (_current_distance > seg.length()) {
        _current_distance -= seg.length();
        if (_path.size() > 1) {
            _path.push_back(_current_edge);
            _prev.push_back(_current_edge);
            _current_edge = _path[1];
            _path.erase(_path.begin());
        }
    }

    {
        float dist = car_offset(_num_cars - 1);
        std::size_t ii = 0;
        while (dist > _current_distance) {
            if (ii >= _prev.size()) {
                break;
            }
            seg = get_world()->rail_network().get_segment(int(_prev[_prev.size() - ii - 1]));
            dist -= seg.length();
            ++ii;
        }
        for (std::size_t sz = _prev.size(); ii < sz; ++ii) {
            _prev.erase(_prev.begin());
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
    float segment_distance = -_current_distance;
    float maximum_speed = max_speed;

    for (std::size_t ii = 0; ii < _path.size(); ++ii) {
        clothoid::segment segment = get_world()->rail_network().get_segment(_path[ii]);

        float s0, s1, k0, k1;

        if (segment_distance < 0) {
            s0 = 0;
            k0 = segment.evaluate_curvature(-segment_distance);
        } else {
            s0 = segment_distance;
            k0 = segment.initial_curvature();
        }

        float v0 = sqrt(max_lateral_acceleration / abs(k0));
        maximum_speed = min(maximum_speed, sqrt(v0 * v0 + 2.f * s0 * max_deceleration));

        if (segment_distance + segment.length() > stopping_distance) {
            s1 = stopping_distance;
            k1 = segment.evaluate_curvature(stopping_distance - segment_distance);
        } else {
            s1 = segment_distance + segment.length();
            k1 = segment.final_curvature();
        }

        float v1 = sqrt(max_lateral_acceleration / abs(k1));
        maximum_speed = min(maximum_speed, sqrt(v1 * v1 + 2.f * s1 * max_deceleration));

        segment_distance += segment.length();
        if (segment_distance > stopping_distance) {
            break;
        }
    }

    if (segment_distance < stopping_distance) {
        float v = sqrt(2.f * segment_distance * max_deceleration);
        maximum_speed = min(maximum_speed, v);
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
