// g_player.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_player.h"
#include "g_navigation.h"
#include "g_shield.h"
#include "g_ship.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type player::_type(object::_type);

//------------------------------------------------------------------------------
player::player()
    : _view({vec2_zero, vec2(640.f, 480.0f)})
    , _usercmd({})
    , _usercmd_time(time_delta::zero)
{
    _view.origin = vec2_zero;
    _view.size = vec2(640.f, 480.f);
}

//------------------------------------------------------------------------------
player::~player()
{
}

//------------------------------------------------------------------------------
void player::spawn()
{
}

//------------------------------------------------------------------------------
void player::draw(render::system* renderer, time_value time) const
{
    (void)renderer;
    (void)time;
}

//------------------------------------------------------------------------------
void player::think()
{
}

//------------------------------------------------------------------------------
vec2 player::get_position(time_value time) const
{
    (void)time;
    return vec2_zero;
}

//------------------------------------------------------------------------------
rot2 player::get_rotation(time_value time) const
{
    (void)time;
    return rot2_identity;
}

//------------------------------------------------------------------------------
mat3 player::get_transform(time_value time) const
{
    (void)time;
    return mat3_identity;
}

//------------------------------------------------------------------------------
player_view player::view(time_value time) const
{
    (void)time;
    return _view;
}

//------------------------------------------------------------------------------
void player::set_aspect(float aspect)
{
    _view.size.x = _view.size.y * aspect;
}

//------------------------------------------------------------------------------
void player::update_usercmd(usercmd cmd, time_value time)
{
    constexpr float zoom_speed = 1.f + (1.f / 4.f);
    constexpr float scroll_speed = 1.f;

    float delta_time = (time - _usercmd_time).to_seconds();

    if (!!(_usercmd.buttons & usercmd::button::scroll_up)) {
        _view.origin.y += scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_down)) {
        _view.origin.y -= scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_left)) {
        _view.origin.x -= scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_right)) {
        _view.origin.x += scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::zoom_in)) {
        _view.size *= exp(-zoom_speed * delta_time);
    }
    if (!!(_usercmd.buttons & usercmd::button::zoom_out)) {
        _view.size *= exp(zoom_speed * delta_time);
    }
    if (!!(_usercmd.buttons & usercmd::button::pan)) {
        _view.origin -= (cmd.cursor - _usercmd.cursor) * _view.size;
    }

    _usercmd = cmd;
    _usercmd_time = time;

    if (_usercmd.action == usercmd::action::zoom_in) {
        _view.size *= (1.f / zoom_speed);
    } else if (_usercmd.action == usercmd::action::zoom_out) {
        _view.size *= zoom_speed;
    }
}

} // namespace game
