// g_player.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_player.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type player::_type(object::_type);

//------------------------------------------------------------------------------
player::player()
    : _usercmd{}
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

    vec2 cursor = (_usercmd.cursor - vec2(.5f)) * renderer->view().size + renderer->view().origin;
    float sz = renderer->view().size.length() * (1.f / 512.f);

    clothoid::network::edge_index edge;
    float length;

    if (get_world()->rail_network().get_closest_segment(cursor, 8.f, edge, length)) {
        renderer->draw_box(vec2(sz), cursor, color4(0,1,0,1));
        vec2 spos = get_world()->rail_network().get_segment(edge).evaluate(length);
        renderer->draw_box(vec2(sz), spos, color4(1,0,0,1));
    } else {
        renderer->draw_box(vec2(sz), cursor, color4(1,1,0,1));
    }
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
float player::get_rotation(time_value time) const
{
    (void)time;
    return 0.f;
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
    constexpr float zoom_speed = 1.f + (1.f / 32.f);
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
