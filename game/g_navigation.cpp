// g_navigation.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_navigation.h"
#include "g_ship.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type navigation::_type(subsystem::_type);

//------------------------------------------------------------------------------
navigation::navigation(game::ship* owner)
    : subsystem(owner, {subsystem_type::navigation, 1})
{}

//------------------------------------------------------------------------------
navigation::~navigation() {}

//------------------------------------------------------------------------------
void navigation::spawn()
{
    object::spawn();
}

//------------------------------------------------------------------------------
void navigation::draw(render::system* renderer, time_value time) const
{
    update_obstacles(renderer, time);
}

//------------------------------------------------------------------------------
void navigation::think()
{
    subsystem::think();

    if (!current_power()) {
        return;
    }

    auto ship = _owner->cast<game::ship>();
    auto engines = ship ? ship->engines() : nullptr;

    if (engines)
    {
        float linear_speed = engines->maximum_linear_speed();
        float angular_speed = engines->maximum_angular_speed();
        vec2 target_position = ship->get_position();

        while (_waypoints.size() && (_waypoints[0] - target_position).length_sqr() < square(32.f)) {
            _waypoints.erase(_waypoints.begin());
        }

        if (_waypoints.size()) {
            target_position = _waypoints[0];
        }

        if (linear_speed && angular_speed && target_position != ship->get_position()) {
            // Target position in ship-local space
            vec2 local = target_position * ship->get_inverse_transform();

            // Radius of circle tangent to the current position and velocity
            // which also contains the target position. This will be infinite
            // if the ship is pointing directly towards the target position.
            float target_radius = std::abs(local.dot(local) / (2.f * local.y));

            // Decrease linear speed if turn radius is too large
            float turn_radius = linear_speed / angular_speed;
            linear_speed *= min(1.f, target_radius / turn_radius);

            // Angle of arc between current and target position on circle
            float arc_angle = std::atan2(local.x, target_radius - std::abs(local.y));
            arc_angle = std::fmod(arc_angle + 2.f * math::pi<float>, 2.f * math::pi<float>);

            // Length of the arc between current and target position
            float arc_length = target_radius * arc_angle;
            // Ratio of the arc length to straight line distance
            float arc_ratio = arc_length / local.length();

            // Decrease angular speed if turn radius is too small, unless the
            // waypoint is far enough away to use a straight-line course.
            angular_speed *= clamp(turn_radius / target_radius, 1.f - std::exp(100.f * (1.f - arc_ratio)), 1.f);
        }

        if (target_position == ship->get_position()) {
            engines->set_target_velocity(vec2_zero, 0);
            _ideal_velocity = vec2_zero;
        } else {
            vec2 delta_move = target_position * ship->get_inverse_transform();
            vec2 move_target = vec2(linear_speed,0) * ship->get_rotation();

            _ideal_velocity = move_target;
            engines->set_target_velocity(move_target, std::copysign(angular_speed, delta_move.y));
        }
    }

    update_obstacles(nullptr, get_world()->frametime());
}

//------------------------------------------------------------------------------
void navigation::read_snapshot(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void navigation::write_snapshot(network::message& /*message*/) const
{
}

//------------------------------------------------------------------------------
void navigation::update_obstacles(render::system* renderer, time_value time) const
{
    // get a list of all ships in the world
    std::vector<game::ship const*> ships;
    for (auto const* object : get_world()->objects()) {
        if (object != _owner && object->is_type<ship>()) {
            if (!object->cast<ship>()->is_destroyed()) {
                ships.push_back(object->cast<ship>());
            }
        }
    }

    auto owner_ship = _owner->cast<game::ship>();
    auto engines = owner_ship ? owner_ship->engines() : nullptr;
    //float mximum_velocity = engines ? engines->maximum_linear_speed() : 0.f;
    float maximum_acceleration = engines ? engines->maximum_linear_acceleration() : 0.f;
    vec2 target_velocity = _ideal_velocity;

    for (auto const* ship : ships) {
        vec2 relative_position = ship->get_position(time) - _owner->get_position(time);
        vec2 relative_velocity = ship->get_linear_velocity() - _owner->get_linear_velocity();

        // find t s.t. |relative_position + t * relative_velocity|^2 < r^2
        float a = relative_velocity.dot(relative_velocity);
        float b = 2.f * relative_position.dot(relative_velocity);
        float c = relative_position.dot(relative_position) - 256.f * 256.f;
        float d = b * b - 4.f * a * c;

        // no collision
        if (d < 0.f) {
            //continue;
        }

        float q = -.5f * (b + std::copysign(std::sqrt(d), b));
        auto t12 = std::minmax({q / a, c / q});
        if (t12.second < 0.f) {
            //continue;
        }

#if 0
        float t = std::max(0.f, t12.first);

        renderer->draw_arc(
            _owner->get_position(time) + _owner->get_linear_velocity() * t,
            128.f,
            0.f,
            -math::pi<float>,
            math::pi<float>,
            color4(1,0,0,1));
        renderer->draw_line(
            _owner->get_position(time),
            _owner->get_position(time) +  _owner->get_linear_velocity() * t
            -  _owner->get_linear_velocity().normalize() * 128.f,
            color4(1,0,0,1),
            color4(1,0,0,1));
#endif

        //rot2 halfangle = rot2(std::asin(256.f / relative_position.length()));
        rot2 halfangle(0, min(1.f, 256.f / relative_position.length()));
        halfangle[0] = std::sqrt(1.f - square(halfangle.y));
        vec2 dir1 = (relative_position * halfangle).normalize();
        vec2 dir2 = (relative_position / halfangle).normalize();

        vec2 apex = ship->get_linear_velocity();

        if (renderer) {
            renderer->draw_line(_owner->get_position(time) + apex, _owner->get_position(time) + apex + dir1 * 64.f, color4(1,1,0,1), color4(1,1,0,0));
            renderer->draw_line(_owner->get_position(time) + apex, _owner->get_position(time) + apex + dir2 * 64.f, color4(1,1,0,1), color4(1,1,0,0));
            renderer->draw_line(_owner->get_position(time), _owner->get_position(time) + _owner->get_linear_velocity(), color4(0,1,0,1), color4(0,1,0,1));
            {
                vec2 vtx[3] = {
                    _owner->get_position(time) + apex,
                    _owner->get_position(time) + apex + dir1 * 64.f,
                    _owner->get_position(time) + apex + dir2 * 64.f,
                };
                color4 col[3] = {
                    color4(1,1,0,.1f),
                    color4(1,1,0,.1f),
                    color4(1,1,0,.1f),
                };
                const int idx[3] = {1, 0, 2};
                renderer->draw_triangles(vtx, col, idx, 3);
            }
#if 0
            //
            //  | r0 + t v |^2 = d^2
            //
            //  v = (v' - apex + s * dir1)
            //
            //
            // (r0 + v' t - t apex) + (-t s * dir1)
            //
            // r1 = r0 + v' t - tapex
            // r1^2 + 2 r1 s dir + t^2 s^2 dir1^2

            auto solve_t = [](vec2 relative_position, vec2 velocity, vec2 apex, vec2 dir, float t, float radius) {
                vec2 r1 = relative_position + velocity * t - apex * t;
                vec2 v1 = -dir * t;

                float a = v1.dot(v1);
                float b = 2.f * r1.dot(v1);
                float c = r1.dot(r1) - radius * radius;
                float d = b * b - 4.f * a * c;

                // no collision
                if (d < 0.f) {
                    return std::pair<float, float>{-FLT_MAX, -FLT_MAX};
                }

                float q = -.5f * (b + std::copysign(std::sqrt(d), b));
                return std::minmax({q / a, c / q});
            };

            for (int jj = 0; jj < 8; ++jj) {
                vec2 pts[32];
                int n = 0;

                for (int ii = 0; ii < 32; ++ii) {
                    float lerp = ii / 31.f;
                    float epsilon = 1e-6f;
                    float l0 = (1.f - epsilon) - (2.f - 2.f * epsilon) * lerp; (void) l0;
                    rot2 angle = rot2(halfangle.radians() * ((1.f - epsilon) - (2.f - 2.f * epsilon) * lerp));
                    vec2 dir = (relative_position * angle).normalize();
                    auto s = solve_t(relative_position, ship->get_linear_velocity(), apex, dir, t * (jj + 1) / 8.f, 256.f);
                    if (s.first > 0.f) {
                        pts[n++] = apex + dir * s.first;
                    }
                }

                for (int ii = 0; ii + 1 < n; ++ii) {
                    vec2 v0 = pts[ii];
                    vec2 v1 = pts[ii + 1];
                    renderer->draw_line(_owner->get_position(time) + v0, _owner->get_position(time) + v1, color4(1,.5f,0,1), color4(1,.5f,0,1));
                }

                if (n > 2) {
                    float M11 = mat3(pts[0].x, pts[0].y, 1.f,
                                    pts[1].x, pts[1].y, 1.f,
                                    pts[2].x, pts[2].y, 1.f).determinant();
                    float M12 = mat3(dot(pts[0], pts[0]), pts[0].y, 1.f,
                                    dot(pts[1], pts[1]), pts[1].y, 1.f,
                                    dot(pts[2], pts[2]), pts[2].y, 1.f).determinant();
                    float M13 = mat3(dot(pts[0], pts[0]), pts[0].x, 1.f,
                                    dot(pts[1], pts[1]), pts[1].x, 1.f,
                                    dot(pts[2], pts[2]), pts[2].x, 1.f).determinant();

                    vec2 xy = .5f * vec2(M12 / M11, -M13 / M11);
                    float r0 = length(xy - pts[0]);
                    renderer->draw_arc(_owner->get_position(time) + xy, r0, 0.f, 0.f, 2.f * math::pi<float>, color4(.5f,1,0,.5f));
                }
            }
#endif
        }

        //
        // check if ideal velocity is in velocity obstacle
        //

        float d1 = (target_velocity - apex).dot(dir1.cross(-1.f));
        float d2 = (target_velocity - apex).dot(dir2.cross( 1.f));

        (void)maximum_acceleration;
        //float collision_time = -min(d1, d2) / maximum_acceleration;
        //if (collision_time > 1.f) {
        //    continue;
        //}

        if (d1 < 0.f && d2 < 0.f) {
            if (d1 < d2) {
                target_velocity -= dir2.cross(1.f) * d2;
            } else {
                target_velocity += dir1.cross(1.f) * d1;
            }
        }
    }

    if (renderer) {
        renderer->draw_line(_owner->get_position(time),
                            _owner->get_position(time) + _ideal_velocity,
                            color4(0,1,1,1), color4(0,1,1,1));
        renderer->draw_line(_owner->get_position(time),
                            _owner->get_position(time) + target_velocity,
                            color4(1,0,1,1), color4(1,0,1,1));
    }

    if (engines && target_velocity != _ideal_velocity) {
        vec2 delta_move = target_velocity / _owner->get_rotation();
        vec2 linear_target = vec2(target_velocity.length(), 0) * _owner->get_rotation();
        float angular_target = std::atan2(delta_move.y, delta_move.x) / FRAMETIME.to_seconds();
        const_cast<game::engines*>(engines.get())->set_target_velocity(linear_target, angular_target);
    }
}

} // namespace game
