// p_trace.cpp
//

#include "p_trace.h"
#include "p_material.h"
#include "p_rigidbody.h"
#include "p_shape.h"
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
namespace physics {

//------------------------------------------------------------------------------
trace::trace(rigid_body const* body, vec2 start, vec2 end)
{
    physics::motion body_motion = body->get_motion();
    physics::circle_shape shape(0);
    physics::motion point_motion{&shape};

    vec2 direction = end - start;
    float fraction = 0.0f;

    for (int num_iterations = 0; num_iterations < max_iterations && fraction < 1.0f; ++num_iterations) {
        point_motion.set_position(start + direction * fraction);
        _contact = physics::collide(point_motion, body_motion).get_contact();

        if (_contact.normal.dot(direction) <= 0.0f) {
            _fraction = 1.0f;
            return;
        }

        if (_contact.distance < epsilon) {
            break;
        }

        fraction += _contact.distance / _contact.normal.dot(direction);
        assert(!isnan(fraction));
    }

    _fraction = fraction > 1.0f ? 1.0f : fraction;
    assert(!isnan(_fraction));
}

//------------------------------------------------------------------------------
trace::trace(rigid_body const* body_a, rigid_body const* body_b, float delta_time)
{
    physics::motion motion_a = body_a->get_motion();
    physics::motion motion_b = body_b->get_motion();

    vec2 p0_a = body_a->get_position();
    vec2 p0_b = body_b->get_position();
    float r0_a = body_a->get_rotation();
    float r0_b = body_b->get_rotation();

    vec2 dp_a = body_a->get_linear_velocity() * delta_time;
    vec2 dp_b = body_b->get_linear_velocity() * delta_time;
    float dr_a = body_a->get_angular_velocity() * delta_time;
    float dr_b = body_b->get_angular_velocity() * delta_time;

    // todo: include rotation
    bounds bounds_a = bounds::from_translation(body_a->get_bounds(), dp_a);
    bounds bounds_b = bounds::from_translation(body_b->get_bounds(), dp_b);

    // bodies do not overlap during this time step
    if (!bounds_a.intersects(bounds_b)) {
        _fraction = 1.0f;
        return;
    }

    vec2 direction = dp_b - dp_a;
    float fraction = 0.f;

    if (direction == vec2_zero) {
        _fraction = 1.0f;
        return;
    }

    for (int num_iterations = 0; num_iterations < max_iterations && fraction < 1.0f; ++num_iterations) {
        motion_a.set_position(p0_a + dp_a * fraction);
        motion_a.set_rotation(r0_a + dr_a * fraction);
        motion_b.set_position(p0_b + dp_b * fraction);
        motion_b.set_rotation(r0_b + dr_b * fraction);

        _contact = physics::collide(motion_a, motion_b).get_contact();

        direction = motion_b.get_linear_velocity(_contact.point) * delta_time
                  - motion_a.get_linear_velocity(_contact.point) * delta_time;

        if (_contact.normal.dot(direction) >= 0.0f) {
            _fraction = 1.0f;
            return;
        }

        if (_contact.distance < epsilon) {
            break;
        }

        fraction -= _contact.distance / _contact.normal.dot(direction);
        assert(!isnan(fraction));
    }

    _fraction = fraction > 1.0f ? 1.0f : fraction;
    assert(!isnan(_fraction));
}

} // namespace physics
