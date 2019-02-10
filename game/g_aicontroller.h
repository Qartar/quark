// g_aicontroller.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class ship;

//------------------------------------------------------------------------------
class aicontroller : public object
{
public:
    static const object_type _type;

public:
    aicontroller(ship* target);
    virtual ~aicontroller();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void think() override;

    virtual vec2 get_position(time_value time) const override;
    virtual float get_rotation(time_value time) const override;
    virtual mat3 get_transform(time_value time) const override;

protected:
    handle<ship> _ship;

    time_value _destroyed_time;

    static constexpr time_delta respawn_time = time_delta::from_seconds(3.f);
};

} // namespace game
