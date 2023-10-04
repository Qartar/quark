// g_rail_station.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class rail_network;

//------------------------------------------------------------------------------
class rail_station : public object
{
public:
    static const object_type _type;

public:
    rail_station();
    virtual ~rail_station();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const;
    virtual void think() override;

    virtual vec2 get_position(time_value time) const override;
    virtual float get_rotation(time_value time) const override;
    virtual mat3 get_transform(time_value time) const override;

protected:

};

} // namespace game
