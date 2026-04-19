// g_projectile.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
struct projectile_info
{
    float damage; //!< damage of the projectile
    float speed; //!< launch speed of the projectile
    float diameter; //!< 
    float ballistic_coefficient;

    effect_type launch_effect;
    sound::asset launch_sound;

    effect_type flight_effect;
    sound::asset flight_sound;

    effect_type impact_effect;
    sound::asset impact_sound;
};

//------------------------------------------------------------------------------
class projectile : public object
{
public:
    static const object_type _type;

public:
    projectile(object* owner, projectile_info info, vec3 position, vec3 velocity);
    ~projectile();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual bool touch(object *other, physics::collision const* collision) override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    float damage() const { return _info.damage; }

    static physics::circle_shape _shape;
    static physics::material _material;

protected:
    projectile_info _info;

    vec3 _position;
    vec3 _velocity;

    time_value _impact_time;

    sound::channel* _channel;
};

} // namespace game
