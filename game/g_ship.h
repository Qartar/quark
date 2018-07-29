// g_ship.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class shield;
class weapon;

//------------------------------------------------------------------------------
class ship : public object
{
public:
    ship();
    ~ship();

    virtual void draw(render::system* renderer, time_value time) const override;
    virtual bool touch(object *other, physics::collision const* collision) override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    void update_usercmd(game::usercmd usercmd);

protected:
    game::usercmd _usercmd;

    shield* _shield;
    std::vector<weapon*> _weapons;

    static physics::material _material;
    static physics::convex_shape _shape;
};

} // namespace game
