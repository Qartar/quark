// g_object.h
//

#pragma once

#include "cm_random.h"
#include "cm_time.h"
#include "g_handle.h"
#include "p_material.h"
#include "p_rigidbody.h"
#include "p_shape.h"

#include <array>

namespace network {
class message;
} // namespace network

namespace physics {
struct collision;
} // namespace physics

namespace render {
class model;
class system;
} // namespace render

////////////////////////////////////////////////////////////////////////////////
namespace game {

class world;

//------------------------------------------------------------------------------
class object_type
{
public:
    object_type();
    //! Construct type object for type with base class
    object_type(object_type const& base);

    //! Returns `true` if this type is derived from `other_type`
    bool is_type(object_type const& other_type) const {
        return _type_index >= other_type._type_index
            && _type_index <= other_type._type_index + other_type._num_derived;
    }

protected:
    //! Index of this type in the type list
    std::size_t _type_index;
    //! Number of types that are derived directly or indirectly from this type
    std::size_t _num_derived;

    object_type* _link; //!< Link to child type for out-of-order initialization
    object_type* _next; //!< Link to sibling type for out-of-order initialization

    static constexpr std::size_t _max_types = 16;
    static std::array<object_type*, _max_types> _types;
    static std::size_t _num_types;

protected:
    void link(object_type const& base);
    static void insert(std::size_t type_index, std::size_t base_index);
};

//------------------------------------------------------------------------------
class object
{
public:
    static const object_type _type;

public:
    object(object* owner = nullptr);
    virtual ~object() {}

    void spawn(); //!< Note: not virtual

    render::model const* model() const { return _model; }

    uint64_t get_sequence() const { return _self.get_sequence(); }

    //! Returns `true` if this type is derived from `other_type`
    bool is_type(object_type const& other_type) const {
        return type().is_type(other_type);
    }

    //! Returns `true` if this type is derived from `other_type`
    template<typename other_type> bool is_type() const {
        return type().is_type(other_type::_type);
    }

    virtual object_type const& type() const { return _type; }
    virtual void draw(render::system* renderer, time_value time) const;
    virtual bool touch(object *other, physics::collision const* collision);
    virtual void think();

    virtual void read_snapshot(network::message const& message);
    virtual void write_snapshot(network::message& message) const;

    //! Get frame-interpolated position
    virtual vec2 get_position(time_value time) const;

    //! Get frame-interpolated rotation
    virtual float get_rotation(time_value time) const;

    //! Get frame-interpolated transform matrix
    virtual mat3 get_transform(time_value time) const;

    //! Get frame-interpolated inverse transform matrix
    virtual mat3 get_inverse_transform(time_value time) const;

    //! Get owner object, or empty handle if none
    handle<object> owner() const { return _owner; }

    physics::rigid_body const& rigid_body() const { return _rigid_body; }

    void set_position(vec2 position, bool teleport = false);
    void set_rotation(float rotation, bool teleport = false);
    void set_linear_velocity(vec2 linear_velocity) { _rigid_body.set_linear_velocity(linear_velocity); }
    void set_angular_velocity(float angular_velocity) { _rigid_body.set_angular_velocity(angular_velocity); }

    vec2 get_position() const { return _rigid_body.get_position(); }
    float get_rotation() const { return _rigid_body.get_rotation(); }
    mat3 get_transform() const { return _rigid_body.get_transform(); }
    mat3 get_inverse_transform() const { return _rigid_body.get_inverse_transform(); }
    vec2 get_linear_velocity() const { return _rigid_body.get_linear_velocity(); }
    float get_angular_velocity() const { return _rigid_body.get_angular_velocity(); }

    void apply_impulse(vec2 impulse) { _rigid_body.apply_impulse(impulse); }
    void apply_impulse(vec2 impulse, vec2 position) { _rigid_body.apply_impulse(impulse, position); }

protected:
    friend world;
    template<typename> friend class handle;

    render::model const* _model;
    color4 _color;

    vec2 _old_position;
    float _old_rotation;

    handle<object> _owner;
    handle<object> _self;
    time_value _spawn_time;

    random _random;

    physics::rigid_body _rigid_body;

    static physics::material _default_material;
    static physics::circle_shape _default_shape;
    constexpr static float _default_mass = 1.0f;

protected:
    game::world* get_world() const { return _self.get_world(); }
};

} // namespace game
