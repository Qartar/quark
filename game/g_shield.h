// g_shield.h
//

#pragma once

#include "g_subsystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
class shield : public subsystem
{
public:
    shield(physics::shape const* base, game::ship* owner);
    ~shield();

    void spawn();

    virtual void draw(render::system* renderer, time_value time) const override;
    virtual bool touch(object *other, physics::collision const* collision) override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    void recharge(float strength_per_second);
    bool damage(vec2 position, float damage);
    float strength() const { return _strength; }
    std::array<color4, 4> const& colors() const { return _colors; }

protected:
    static physics::material _material;
    physics::shape const* _base;
    physics::convex_shape _shape;

    std::array<color4, 4> _colors;

    float _strength;

    time_value _damage_time; //!< last time the shield received damage
    static constexpr time_delta recharge_delay = time_delta::from_seconds(0.5f); //!< delay after damage before shield recharge
    static constexpr float discharge_rate = 1.f; //!< strength per second discharged when current strength exceeds power level

    static constexpr int kNumVertices = 64;
    vec2 _vertices[kNumVertices];
    float _flux[kNumVertices];

    // for draw interpolation
    float _prev_strength;
    float _prev_flux[kNumVertices];

protected:
    void step_vertices();
    void step_strength();
};

} // namespace game
