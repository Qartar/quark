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
    mat4 transform;
    vec2 origin;
    vec2 size;
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
    virtual rot2 get_rotation(time_value time) const override;
    virtual mat3 get_transform(time_value time) const override;

    player_view view(time_value time, time_value realtime) const;

    void set_aspect(float aspect);
    virtual void update_usercmd(usercmd cmd, time_value realtime);

protected:
    player_view _view;
    usercmd _usercmd;
    time_value _usercmd_time; //!< realtime, not frametime

    time_value _timescale_time; //!< realtime since game speed was changed

    handle<ship> _hover;
    handle<ship> _follow;

    std::vector<handle<ship>> _selection;
    time_value _selection_time; //!< Last completed selection, used for double-click/follow
    vec2 _selection_start; //!< Start of drag select rectangle in world space
    bool _is_selecting; //!< True if currently in a drag select

protected:
    void draw_selection(render::system* renderer, time_value time, std::vector<handle<ship>> const& selection) const;

    handle<ship> hover_target(vec2 cursor) const;
    std::vector<handle<ship>> selection_target(vec2 cursor) const;
    void on_select(vec2 cursor);
};

} // namespace game
