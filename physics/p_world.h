// p_world.h
//

#pragma once

#include "cm_vector.h"
#include "p_collide.h"
#include <functional>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace physics {

class rigid_body;
class trace;
class shape;

//------------------------------------------------------------------------------
struct collision : contact
{
    vec2 impulse; //!< Impulse that should be applied to body_b to resolve the collision

    collision() = default;
    explicit collision(contact const& c) : contact(c) {}
};

//------------------------------------------------------------------------------
class world
{
public:
    using filter_callback_type = std::function<bool(physics::rigid_body const* body_a, physics::rigid_body const* body_b)>;
    using collision_callback_type = std::function<bool(physics::rigid_body const* body_a, physics::rigid_body const* body_b, physics::collision const& collision)>;

    world(filter_callback_type filter_callback, collision_callback_type collision_callback);

    void add_body(physics::rigid_body* body);
    void remove_body(physics::rigid_body* body);

    void step(float delta_time);

    struct trace_result
    {
        physics::contact c; //!< Trace intersection
        float fraction; //!< Fraction along trace
        physics::rigid_body* body; //!< Intersected body
    };

    //! Trace through all bodies in the world from `start` to `end` and return
    //! a list of contacts in `results` sorted by trace fraction.
    std::size_t trace(vec2 start,
                      vec2 end,
                      trace_result* results,
                      std::size_t max_results) const;

protected:
    std::vector<physics::rigid_body*> _bodies;

    filter_callback_type _filter_callback;
    collision_callback_type _collision_callback;

protected:
    vec2 collision_impulse(physics::rigid_body const* body_a,
                           physics::rigid_body const* body_b,
                           physics::contact const& contact) const;

    using overlap = std::pair<std::size_t, std::size_t>;

    //! Return a lexicographically sorted list of all pairs of bodies which
    //! overlap during the next `delta_time` step, including permutations.
    std::vector<overlap> generate_overlaps(float delta_time) const;
};

} // namespace physics
