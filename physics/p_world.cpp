// p_world.cpp
//

#include "p_world.h"
#include "p_collide.h"
#include "p_material.h"
#include "p_rigidbody.h"
#include "p_trace.h"
#include "cm_shared.h"

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
void world::step(double delta_time)
{
    struct candidate {
        std::size_t body_b;
        double fraction;
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
            if (tr.get_fraction() == 1.0) {
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
            assert(!isnan(c.impulse));

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
        _bodies[ii]->set_rotation(_bodies[ii]->get_rotation() * rot3(_bodies[ii]->get_angular_velocity() * delta_time));
    }
}

//------------------------------------------------------------------------------
std::size_t world::trace(
    vec3 start,
    vec3 end,
    trace_result* results,
    std::size_t max_results) const
{
    bounds3 trace_bounds = bounds3::from_points({start, end});
    std::size_t num_results = 0;

    // TODO: spatial acceleration
    for (std::size_t ii = 0; ii < _bodies.size(); ++ii) {
        if (!trace_bounds.intersects(_bodies[ii]->get_bounds())) {
            continue;
        }

        auto tr = physics::trace(_bodies[ii], start, end);
        if (tr.get_fraction() == 1.0) {
            continue;
        }

        // insertion sort
        std::size_t jj = 0;
        for (; jj < num_results; ++jj) {
            if (tr.get_fraction() < results[jj].fraction) {
                break;
            }
        }

        if (jj >= max_results) {
            continue;
        }

        if (num_results < max_results) {
            results[num_results].c = tr.get_contact();
            results[num_results].fraction = tr.get_fraction();
            results[num_results].body = _bodies[ii];
            std::rotate(results + jj, results + num_results, results + num_results + 1);
            ++num_results;
        } else {
            std::rotate(results + jj, results + max_results - 1, results + max_results);
            results[jj].c = tr.get_contact();
            results[jj].fraction = tr.get_fraction();
            results[jj].body = _bodies[ii];
        }
    }

    return num_results;
}

//------------------------------------------------------------------------------
physics::rigid_body* world::point_query(vec3 point) const
{
    // TODO: spatial acceleration
    for (std::size_t ii = 0; ii < _bodies.size(); ++ii) {
        if (!_bodies[ii]->get_bounds().contains(point)) {
            continue;
        }

        if (_bodies[ii]->contains_point(point)) {
            return _bodies[ii];
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------------
std::size_t world::bounds_query(bounds b, mat4 projection, physics::rigid_body** bodies, std::size_t max_bodies) const
{
    // TODO: spatial acceleration
    std::size_t num_bodies = 0;
    for (std::size_t ii = 0; num_bodies < max_bodies && ii < _bodies.size(); ++ii) {
        if (_bodies[ii]->get_bounds(projection).intersects(b)) {
            bodies[num_bodies++] = _bodies[ii];
        }
    }

    return num_bodies;
}

//------------------------------------------------------------------------------
vec3 world::collision_impulse(
    physics::rigid_body const* body_a,
    physics::rigid_body const* body_b,
    physics::contact const& contact) const
{
    vec3 position = contact.point;
    vec3 direction = contact.normal;
    double distance = contact.distance;

    // Calculate the relative velocity of the bodies at the contact point
    vec3 relative_velocity = body_b->get_linear_velocity(position)
                           - body_a->get_linear_velocity(position);

    // Simple collision response for penetrating bodies
    if (distance >= 0.0 || relative_velocity.dot(direction) >= 0.0) {
        return vec3_zero;
    }

    vec3 tangent = (relative_velocity - direction * relative_velocity.dot(direction)).normalize();

    // Use the geometric mean of both bodies' coefficient of restitution
    double restitution = sqrt(body_a->get_material()->restitution()
                           * body_b->get_material()->restitution());

    // Use the geometric mean of both bodies' coefficient of friction
    double mu = sqrt(body_a->get_material()->contact_friction()
                  * body_b->get_material()->contact_friction());

    // Calculate the inverse reduced mass of both bodies
    double inverse_reduced_mass = body_a->get_inverse_mass()
                               + body_b->get_inverse_mass();

    vec3 ra = position - body_a->get_position();
    vec3 rb = position - body_b->get_position();

    // Change in normal velocity per change in momentum along normal
    double gx = inverse_reduced_mass
             + body_a->get_inverse_inertia() * ra.cross(direction).length_sqr()
             + body_b->get_inverse_inertia() * rb.cross(direction).length_sqr();

    // Change in tangent velocity per change in momentum along normal
    double gy = body_a->get_inverse_inertia() * ra.cross(direction).cross(ra).dot(-tangent)
             + body_b->get_inverse_inertia() * rb.cross(direction).cross(rb).dot(-tangent);

    // Change in normal velocity per change in momentum along tangent
    double hx = body_a->get_inverse_inertia() * ra.cross(-tangent).cross(ra).dot(direction)
             + body_b->get_inverse_inertia() * rb.cross(-tangent).cross(rb).dot(direction);

    // Change in tangent velocity per change in momentum along tangent
    double hy = inverse_reduced_mass
             + body_a->get_inverse_inertia() * ra.cross(-tangent).length_sqr()
             + body_b->get_inverse_inertia() * rb.cross(-tangent).length_sqr();

    double dvx = -(1.0 + restitution) * relative_velocity.dot(direction);
    double dvy = -relative_velocity.dot(-tangent);

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

    double inv_det = 1.0 / (gx * hy - hx * gy);

    double dpx = inv_det * ( hy * dvx - hx * dvy);
    double dpy = inv_det * (-gy * dvx + gx * dvy);

    // Clamp friction impulse by friction coefficient
    if (std::abs(dpy) > mu * std::abs(dpx)) {
        // Find clamped vy using the original vector equation with dpy := mu * dpx
        double dvy0 = (gy + mu * hy) / (gx + mu * hx) * dvx;

        // Recalculate impulse using clamped friction
        dpx = inv_det * ( hy * dvx - hx * dvy0);
        dpy = inv_det * (-gy * dvx + gx * dvy0);
    }

    return direction * dpx - tangent * dpy;
}

//------------------------------------------------------------------------------
std::vector<world::overlap> world::generate_overlaps(double delta_time) const
{
    std::vector<bounds3> swept_bounds(_bodies.size());
    bounds3 combined_bounds(vec3(DBL_MAX), vec3(-DBL_MAX));
    for (std::size_t ii = 0, sz = _bodies.size(); ii < sz; ++ii) {
        // todo: include rotation
        swept_bounds[ii] = bounds3::from_translation(_bodies[ii]->get_bounds(),
                                                     _bodies[ii]->get_linear_velocity() * delta_time);
        combined_bounds |= swept_bounds[ii];
    }

    std::vector<overlap> overlaps;
    std::vector<size_t> sorted(_bodies.size());
    std::iota(sorted.begin(), sorted.end(), 0);

    // sweep along the largest axis
    vec3 combined_size = combined_bounds.size();
    int axis = max3index(combined_size.x, combined_size.y, combined_size.z);

    // sort bounds on the current axis
    std::sort(sorted.begin(), sorted.end(),
        [&swept_bounds, axis](std::size_t lhs, std::size_t rhs) {
            return swept_bounds[lhs][0][axis] < swept_bounds[rhs][0][axis];
        });

    // generate overlaps on the current axis
    for (std::size_t ii = 0, sz = _bodies.size(); ii < sz; ++ii) {
        bounds3 b = swept_bounds[sorted[ii]];
        for (std::size_t jj = ii + 1; jj < sz; ++jj) {
            if (b[1][axis] < swept_bounds[sorted[jj]][0][axis]) {
                break;
            }

            if (!b.intersects(swept_bounds[sorted[jj]])) {
                continue;
            }

            // check collision filter, note: filter is not necessarily symmetric
            if (!_filter_callback || _filter_callback(_bodies[sorted[ii]], _bodies[sorted[jj]])) {
                overlaps.push_back({sorted[ii], sorted[jj]});
            }
            if (!_filter_callback || _filter_callback(_bodies[sorted[jj]], _bodies[sorted[ii]])) {
                overlaps.push_back({sorted[jj], sorted[ii]});
            }
        }
    }

    // sort overlaps on the current axis by body ids
    std::sort(overlaps.begin(), overlaps.end());
    return overlaps;
}

} // namespace physics
