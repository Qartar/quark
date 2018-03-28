// p_collide.h
//

#pragma once

#include "cm_vector.h"
#include "p_motion.h"

////////////////////////////////////////////////////////////////////////////////
namespace physics {

class shape;

//------------------------------------------------------------------------------
struct contact
{
    float distance; //!< Contact distance, negative for penetration
    vec2 point; //!< Contact point in world space
    vec2 normal; //!< Contact normal in world space
};

//------------------------------------------------------------------------------
class collide
{
public:
    collide(motion const& motion_a, motion const& motion_b);

    bool has_contact() const {
        return _has_contact;
    }

    contact const& get_contact() const {
        return _contact;
    }

    static vec2 closest_point(shape const* shape, vec2 point);

protected:
    bool _has_contact;

    contact _contact;

    struct motion_data : motion
    {
        motion_data(motion const&);
        mat3 local_to_world;
        mat3 world_to_local;
    };

    constexpr static int max_iterations = 64;
    constexpr static int max_vertices = 64;
    constexpr static float epsilon = 1e-12f;

    struct support_vertex
    {
        vec3 a; //!< point on object A
        vec3 b; //!< point on object B
        vec3 d; //!< "Minkowski difference"; a - b
    };

    //
    //  GJK
    //

    float minimum_distance(vec3& point, vec3& direction) const;

    support_vertex supporting_vertex(vec3 direction) const;

    vec3 motion_supporting_vertex(motion_data const& motion, vec3 direction) const;

    vec3 nearest_difference(support_vertex a, support_vertex b) const;

    vec3 nearest_point(support_vertex a, support_vertex b) const;

    bool triangle_contains_origin(vec3 a, vec3 b, vec3 c) const;

    //
    //  EPA
    //

    float penetration_distance(support_vertex a, support_vertex b, support_vertex c, vec3& point, vec3& direction) const;

    std::size_t nearest_edge_index(vec3 normal, support_vertex const* vertices, std::size_t num_vertices, vec3& direction) const;

protected:
    motion_data _motion[2];
};

} // namespace physics
