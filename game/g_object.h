// g_object.h
//

#pragma once

#include "cm_random.h"
#include "cm_time.h"
#include "g_handle.h"
#include "p_material.h"
#include "p_rigidbody.h"
#include "p_shape.h"

namespace network {
class message;
} // namespace network

namespace physics {
struct collision;
} // namespace physics

namespace render {
class system;
} // namespace render

////////////////////////////////////////////////////////////////////////////////
namespace game {

class world;

//------------------------------------------------------------------------------
enum class object_type
{
    object,
    obstacle,
    projectile,
    ship,
    module,
    shield,
    weapon,
};

//------------------------------------------------------------------------------
class object
{
public:
    object(object_type type, object* owner = nullptr);
    virtual ~object() {}

    void spawn(); //!< Note: not virtual

    std::size_t spawn_id() const { return _spawn_id; }

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

    render::model const* _model;
    color4 _color;

    vec2 _old_position;
    float _old_rotation;

    object_type _type;

protected:
    friend world;
    template<typename> friend class handle;

    //! Game world which contains this object
    world* _world;

    handle<object> _owner;

    std::size_t _spawn_id;
    time_value _spawn_time;

    random _random;

    physics::rigid_body _rigid_body;

    static physics::material _default_material;
    static physics::circle_shape _default_shape;
    constexpr static float _default_mass = 1.0f;
};

//------------------------------------------------------------------------------
class obstacle : public object
{
public:
    obstacle(physics::rigid_body&& rigid_body)
        : object(object_type::obstacle)
    {
        _rigid_body = std::move(rigid_body);
    }
};

} // namespace game
