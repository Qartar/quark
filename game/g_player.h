// g_player.h
//

#pragma once

#include "g_object.h"
#include "g_usercmd.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class ship;

//------------------------------------------------------------------------------
struct player_view {
    vec2 origin;
    vec2 size;
    float angle;
};

//------------------------------------------------------------------------------
class player : public object
{
public:
    static const object_type _type;

public:
    player();
    virtual ~player();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const;
    virtual void think() override;

    virtual vec2 get_position(time_value time) const override;
    virtual float get_rotation(time_value time) const override;
    virtual mat3 get_transform(time_value time) const override;

    player_view view(time_value time) const;

    void set_aspect(float aspect);
    virtual void update_usercmd(usercmd cmd, time_value time);

protected:
    player_view _view;
    usercmd _usercmd;
    time_value _usercmd_time;

    handle<object> _follow;

protected:
    void on_follow();
};

} // namespace game
