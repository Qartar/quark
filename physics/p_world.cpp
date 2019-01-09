// p_world.cpp
//

#include "p_world.h"
#include "p_collide.h"
#include "p_material.h"
#include "p_rigidbody.h"
#include "p_trace.h"

#include <cassert>
#include <algorithm>
#include <numeric>
#include <queue>

////////////////////////////////////////////////////////////////////////////////
namespace physics {

//------------------------------------------------------------------------------
world::world(filter_callback_type filter_callback, collision_callback_type collision_callback)
    : _filter_callback(filter_callback)
    , _collision_callback(collision_callback)
{
}

//------------------------------------------------------------------------------
void world::add_body(physics::rigid_body* body)
{
    assert(std::find(_bodies.begin(), _bodies.end(), body) == _bodies.end());
    _bodies.push_back(body);
}

//------------------------------------------------------------------------------
void world::remove_body(physics::rigid_body* body)
{
    assert(std::find(_bodies.begin(), _bodies.end(), body) != _bodies.end());
    _bodies.erase(std::find(_bodies.begin(), _bodies.end(), body));
}

//------------------------------------------------------------------------------
void world::step(float delta_time)
{
    struct candidate {
        std::size_t body_b;
        float fraction;
        physics::contact contact;

        bool operator<(candidate const& other) const {
            return fraction > other.fraction; // sort ascending
        }
    };

    // calculate all overlapping body pairs, including permutations
    std::vector<overlap> overlaps = generate_overlaps(delta_time);

    for (std::size_t idx = 0; idx < overlaps.size();) {
        std::size_t ii = overlaps[idx].first;

        std::priority_queue<candidate> candidates;
        // check all bodies overlapping with body index `ii`
        for (; idx < overlaps.size() && overlaps[idx].first == ii; ++idx) {
            std::size_t jj = overlaps[idx].second;

            // check collision
            physics::trace tr(_bodies[ii], _bodies[jj], delta_time);
            if (tr.get_fraction() == 1.f) {
                continue;
            }

            candidates.push({jj, tr.get_fraction(), tr.get_contact()});
        }

        // use the earliest collision candidate
        while (candidates.size()) {
            candidate candidate = candidates.top();
            std::size_t jj = candidate.body_b;
            candidates.pop();

            if (candidates.size()) {
                assert(candidates.top().fraction >= candidate.fraction);
            }

            physics::collision c = physics::collision(candidate.contact);
            c.impulse = collision_impulse(_bodies[ii], _bodies[jj], c);

            // check collision callback
            if (_collision_callback && !_collision_callback(_bodies[ii], _bodies[jj], c)) {
                continue;
            }

            // collision response
            _bodies[ii]->apply_impulse(-c.impulse, c.point);
            _bodies[jj]->apply_impulse( c.impulse, c.point);

            break;
        }
    }

    // move

    for (std::size_t ii = 0; ii < _bodies.size(); ++ii) {
        _bodies[ii]->set_position(_bodies[ii]->get_position() + _bodies[ii]->get_linear_velocity() * delta_time);
        _bodies[ii]->set_rotation(_bodies[ii]->get_rotation() + _bodies[ii]->get_angular_velocity() * delta_time);
    }
}

//------------------------------------------------------------------------------
vec2 world::collision_impulse(
    physics::rigid_body const* body_a,
    physics::rigid_body const* body_b,
    physics::contact const& contact) const
{
    vec3 position = vec3(contact.point);
    vec3 direction = vec3(contact.normal);
    float distance = contact.distance;

    // Calculate the relative velocity of the bodies at the contact point
    vec3 relative_velocity = vec3(body_b->get_linear_velocity(position.to_vec2()))
                           - vec3(body_a->get_linear_velocity(position.to_vec2()));

    // Simple collision response for penetrating bodies
    if (distance >= 0.0f || relative_velocity.dot(direction) >= 0.f) {
        return vec2_zero;
    }

    vec3 tangent = (relative_velocity - direction * relative_velocity.dot(direction)).normalize();

    // Use the geometric mean of both bodies' coefficient of restitution
    float restitution = sqrt(body_a->get_material()->restitution()
                           * body_b->get_material()->restitution());

    // Use the geometric mean of both bodies' coefficient of friction
    float mu = sqrt(body_a->get_material()->contact_friction()
                  * body_b->get_material()->contact_friction());

    // Calculate the inverse reduced mass of both bodies
    float inverse_reduced_mass = body_a->get_inverse_mass()
                               + body_b->get_inverse_mass();

    vec3 ra = position - vec3(body_a->get_position());
    vec3 rb = position - vec3(body_b->get_position());

    // Change in normal velocity per change in momentum along normal
    float gx = inverse_reduced_mass
             + body_a->get_inverse_inertia() * ra.cross(direction).length_sqr()
             + body_b->get_inverse_inertia() * rb.cross(direction).length_sqr();

    // Change in tangent velocity per change in momentum along normal
    float gy = body_a->get_inverse_inertia() * ra.cross(direction).cross(ra).dot(-tangent)
             + body_b->get_inverse_inertia() * rb.cross(direction).cross(rb).dot(-tangent);

    // Change in normal velocity per change in momentum along tangent
    float hx = body_a->get_inverse_inertia() * ra.cross(-tangent).cross(ra).dot(direction)
             + body_b->get_inverse_inertia() * rb.cross(-tangent).cross(rb).dot(direction);

    // Change in tangent velocity per change in momentum along tangent
    float hy = inverse_reduced_mass
             + body_a->get_inverse_inertia() * ra.cross(-tangent).length_sqr()
             + body_b->get_inverse_inertia() * rb.cross(-tangent).length_sqr();

    float dvx = -(1.0f + restitution) * relative_velocity.dot(direction);
    float dvy = -relative_velocity.dot(-tangent);

    // Solve the vector equation:
    //
    //             | Gx  Hx |
    // dV = M dP = |        | dP
    //             | Gy  Hy |

    // Inverting the response matrix gives:
    //
    //           1      |  Hy  -Hx |
    // M' = ----------- |          |
    //      GxHy - HxGy | -Gy   Gx |

    float inv_det = 1.0f / (gx * hy - hx * gy);

    float dpx = inv_det * ( hy * dvx - hx * dvy);
    float dpy = inv_det * (-gy * dvx + gx * dvy);

    // Clamp friction impulse by friction coefficient
    if (std::abs(dpy) > mu * std::abs(dpx)) {
        // Find clamped vy using the original vector equation with dpy := mu * dpx
        float dvy0 = (gy + mu * hy) / (gx + mu * hx) * dvx;

        // Recalculate impulse using clamped friction
        dpx = inv_det * ( hy * dvx - hx * dvy0);
        dpy = inv_det * (-gy * dvx + gx * dvy0);
    }

    return (direction * dpx - tangent * dpy).to_vec2();
}

//------------------------------------------------------------------------------
std::vector<world::overlap> world::generate_overlaps(float delta_time) const
{
    std::vector<bounds> swept_bounds(_bodies.size());
    for (std::size_t ii = 0, sz = _bodies.size(); ii < sz; ++ii) {
        // todo: include rotation
        swept_bounds[ii] = bounds::from_translation(_bodies[ii]->get_bounds(),
                                                    _bodies[ii]->get_linear_velocity() * delta_time);
    }

    std::vector<overlap> axis_overlaps[2];
    std::vector<size_t> sorted(_bodies.size());
    std::iota(sorted.begin(), sorted.end(), 0);

    for (int axis = 0; axis < 2; ++axis) {
        // sort bounds on the current axis
        std::sort(sorted.begin(), sorted.end(),
            [&swept_bounds, axis](std::size_t lhs, std::size_t rhs) {
                return swept_bounds[lhs][0][axis] < swept_bounds[rhs][0][axis];
            });

        // generate overlaps on the current axis
        for (std::size_t ii = 0, sz = _bodies.size(); ii < sz; ++ii) {
            bounds b = swept_bounds[sorted[ii]];
            for (std::size_t jj = ii + 1; jj < sz; ++jj) {
                if (b[1][axis] < swept_bounds[sorted[jj]][0][axis]) {
                    break;
                }

                // check collision filter, note: filter is not necessarily symmetric
                if (!_filter_callback || _filter_callback(_bodies[sorted[ii]], _bodies[sorted[jj]])) {
                    axis_overlaps[axis].push_back({sorted[ii], sorted[jj]});
                }
                if (!_filter_callback || _filter_callback(_bodies[sorted[jj]], _bodies[sorted[ii]])) {
                    axis_overlaps[axis].push_back({sorted[jj], sorted[ii]});
                }
            }
        }

        // sort overlaps on the current axis by body ids
        std::sort(axis_overlaps[axis].begin(), axis_overlaps[axis].end());
    }

    // generate the intersection of overlaps on both axes
    std::vector<overlap> overlaps;
    std::set_intersection(axis_overlaps[0].begin(), axis_overlaps[0].end(),
                          axis_overlaps[1].begin(), axis_overlaps[1].end(),
                          std::back_inserter(overlaps));
    return overlaps;
}

} // namespace physics
