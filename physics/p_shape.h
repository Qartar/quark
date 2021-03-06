//  p_shape.h
//

#pragma once

#include "cm_bounds.h"
#include "cm_vector.h"
#include "cm_matrix.h"
#include "cm_shared.h"

////////////////////////////////////////////////////////////////////////////////
namespace physics {

enum class shape_type
{
    box,
    circle,
    convex,
    compound,
};

//------------------------------------------------------------------------------
class shape
{
public:
    virtual ~shape() {}

    virtual shape_type type() const = 0;

    virtual bool contains_point(vec2 point) const = 0;

    virtual vec2 supporting_vertex(vec2 direction) const = 0;

    virtual float calculate_area() const = 0;

    virtual void calculate_mass_properties(float inverse_mass, vec2& center_of_mass, float& inverse_inertia) const = 0;

    virtual bounds calculate_bounds(mat3 transform) const = 0;
};

//------------------------------------------------------------------------------
class box_shape : public shape
{
public:
    box_shape(vec2 size)
        : _half_size(size * 0.5f)
    {
    }

    virtual shape_type type() const override {
        return shape_type::box;
    }

    virtual bool contains_point(vec2 point) const override {
        return !(point.x < -_half_size.x || point.x > _half_size.x ||
                    point.y < -_half_size.y || point.y > _half_size.y);
    }

    virtual vec2 supporting_vertex(vec2 direction) const override {
        return _half_size * mat2::scale(direction.x < 0.f ? -1.f : 1.f,
                                        direction.y < 0.f ? -1.f : 1.f);
    }

    virtual float calculate_area() const override {
        return 4.f * _half_size.x * _half_size.y;
    }

    virtual void calculate_mass_properties(float inverse_mass, vec2& center_of_mass, float& inverse_inertia) const override {
        center_of_mass = vec2(0,0);
        if (inverse_mass > 0.f) {
            inverse_inertia = 3.0f * inverse_mass / _half_size.dot(_half_size);
        } else {
            inverse_inertia = 0.0f;
        }
    }

    virtual bounds calculate_bounds(mat3 transform) const override {
        return bounds::from_points({
            vec2(-_half_size.x, -_half_size.y) * transform,
            vec2(+_half_size.x, -_half_size.y) * transform,
            vec2(-_half_size.x, +_half_size.y) * transform,
            vec2(+_half_size.x, +_half_size.y) * transform});
    }

protected:
    vec2 _half_size;
};

//------------------------------------------------------------------------------
class circle_shape : public shape
{
public:
    circle_shape(float radius)
        : _radius(radius)
    {
    }

    virtual shape_type type() const override {
        return shape_type::circle;
    }

    virtual bool contains_point(vec2 point) const override {
        return point.dot(point) < _radius * _radius;
    }

    virtual vec2 supporting_vertex(vec2 direction) const override {
        return direction.normalize() * _radius;
    }

    virtual float calculate_area() const override {
        return math::pi<float> * _radius * _radius;
    }

    virtual void calculate_mass_properties(float inverse_mass, vec2& center_of_mass, float& inverse_inertia) const override {
        center_of_mass = vec2(0,0);
        if (inverse_mass > 0.0f) {
            inverse_inertia = 2.0f * inverse_mass / (_radius * _radius);
        } else {
            inverse_inertia = 0.0f;
        }
    }

    virtual bounds calculate_bounds(mat3 transform) const override {
        return bounds(vec2(-_radius), vec2(_radius)) + vec2_zero * transform;
    }

protected:
    float _radius;
};

//------------------------------------------------------------------------------
class convex_shape : public shape
{
public:
    convex_shape();
    convex_shape(vec2 const* vertices, std::size_t num_vertices);
    template<std::size_t sz> convex_shape(vec2 const (&vertices)[sz])
        : convex_shape(vertices, sz)
    {
        static_assert(sz < kMaxVertices - 1, "exceeded maximum vertices");
    }

    virtual shape_type type() const override {
        return shape_type::convex;
    }

    virtual bool contains_point(vec2 point) const override;

    virtual vec2 supporting_vertex(vec2 direction) const override;

    virtual float calculate_area() const override { return _area; }

    virtual void calculate_mass_properties(float inverse_mass, vec2& center_of_mass, float& inverse_inertia) const override;

    virtual bounds calculate_bounds(mat3 transform) const override;

    convex_shape shrink_by_radius(float radius) const;

    convex_shape expand_by_radius(float radius) const;

    static convex_shape from_planes(vec3 const* planes, std::size_t num_planes);

protected:
    static constexpr std::size_t kMaxVertices = 64;
    std::size_t _num_vertices;
    float _area;
    vec2 _center_of_mass;
    vec2 _vertices[kMaxVertices + 1];

protected:
    std::size_t _extract_convex_hull(vec2* vertices, std::size_t num_vertices) const;
    std::size_t _decimate_convex_hull(vec2* vertices, std::size_t num_vertices, std::size_t max_vertices) const;
};

} // namespace physics
