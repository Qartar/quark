// g_projectile.h
//

#pragma once

#include "g_object.h"
#include "r_outline.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

struct gun_design;

//------------------------------------------------------------------------------
class projectile : public object
{
public:
    static const object_type _type;

public:
    projectile(object* owner, gun_design const* design, vec3 position, vec3 velocity);
    ~projectile();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual bool touch(object *other, physics::collision const* collision) override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    static physics::circle_shape _shape;
    static physics::material _material;

protected:
    gun_design const* _design;

    vec3 _position;
    vec3 _velocity;

    time_value _impact_time;

    sound::channel* _channel;

    static constexpr float mass_to_damage = 1.5e-4f;
};

} // namespace game
