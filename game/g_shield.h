// g_shield.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
class shield : public object
{
public:
    shield(physics::shape const* base, game::object* owner);
    ~shield();

    virtual void draw(render::system* renderer, time_value time) const override;
    virtual bool touch(object *other, physics::collision const* collision) override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    void recharge(float strength_per_second);
    bool damage(vec2 position, float damage);

protected:
    static physics::material _material;
    physics::shape const* _base;
    physics::convex_shape _shape;

    int _style;

    static constexpr int kNumVertices = 64;

    vec2 _vertices[kNumVertices];
    float _strength[kNumVertices];
    float _flux[kNumVertices];

protected:
    void step_vertices();
    void step_strength();
};

} // namespace game
