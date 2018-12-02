// g_projectile.h
//

#pragma once

#include "g_object.h"
#include "g_weapon.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
class projectile : public object
{
public:
    projectile(object* owner, float damage, weapon_type type);
    ~projectile();

    void spawn();

    virtual void draw(render::system* renderer, time_value time) const override;
    virtual bool touch(object *other, physics::collision const* collision) override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    float damage() const { return _damage; }

    static physics::circle_shape _shape;
    static physics::material _material;

protected:
    float _damage;

    weapon_type _type;

    time_value _impact_time;

    sound::channel* _channel;

    sound::asset _sound_cannon_impact;
    sound::asset _sound_blaster_impact;
    sound::asset _sound_missile_flight;

    static constexpr time_delta fuse_time = time_delta::from_seconds(5.f);
    static constexpr time_delta fade_time = time_delta::from_seconds(1.f);

protected:
    void update_homing();
    void update_effects();
    void update_sound();
};

} // namespace game
