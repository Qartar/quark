// g_rail_signal.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_rail_signal.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type rail_signal::_type(object::_type);

//------------------------------------------------------------------------------
rail_signal::rail_signal()
    : _state(signal_state::available)
{
}

//------------------------------------------------------------------------------
rail_signal::~rail_signal()
{
}

//------------------------------------------------------------------------------
void rail_signal::spawn()
{
}

//------------------------------------------------------------------------------
void rail_signal::draw(render::system* renderer, time_value time) const
{
    (void)renderer;
    (void)time;
}

//------------------------------------------------------------------------------
void rail_signal::think()
{
}

//------------------------------------------------------------------------------
vec2 rail_signal::get_position(time_value time) const
{
    (void)time;
    return vec2_zero;
}

//------------------------------------------------------------------------------
float rail_signal::get_rotation(time_value time) const
{
    (void)time;
    return 0.f;
}

//------------------------------------------------------------------------------
mat3 rail_signal::get_transform(time_value time) const
{
    (void)time;
    return mat3_identity;
}

} // namespace game
