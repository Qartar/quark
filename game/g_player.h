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
};

//------------------------------------------------------------------------------
class player : public object
{
public:
    static const object_type _type;

public:
    player(ship* target);
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
    void update_usercmd(usercmd cmd, time_value time);

protected:
    handle<ship> _ship;
    player_view _view;
    usercmd _usercmd;
    time_value _usercmd_time;

    std::vector<vec2> _waypoints;
    bool _move_selection;
    bool _move_appending;
    int _weapon_selection;

    time_value _destroyed_time;

    static constexpr time_delta respawn_time = time_delta::from_seconds(3.f);

protected:
    bool attack(vec2 position, bool repeat);
    void toggle_weapon(int weapon_index, bool toggle_power);
};

} // namespace game
