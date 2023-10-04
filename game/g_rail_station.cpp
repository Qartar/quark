// g_rail_station.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_rail_station.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type rail_station::_type(object::_type);

//------------------------------------------------------------------------------
rail_station::rail_station()
{
}

//------------------------------------------------------------------------------
rail_station::~rail_station()
{
}

//------------------------------------------------------------------------------
void rail_station::spawn()
{
}

//------------------------------------------------------------------------------
void rail_station::draw(render::system* renderer, time_value time) const
{
    (void)renderer;
    (void)time;
}

//------------------------------------------------------------------------------
void rail_station::think()
{
}

//------------------------------------------------------------------------------
vec2 rail_station::get_position(time_value time) const
{
    (void)time;
    return vec2_zero;
}

//------------------------------------------------------------------------------
float rail_station::get_rotation(time_value time) const
{
    (void)time;
    return 0.f;
}

//------------------------------------------------------------------------------
mat3 rail_station::get_transform(time_value time) const
{
    (void)time;
    return mat3_identity;
}

} // namespace game
