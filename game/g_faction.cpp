// g_faction.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_faction.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {
    
const object_type faction::_type(object::_type);

//------------------------------------------------------------------------------
faction::faction(string::view name, color4 color)
    : _name(name)
    , _color(color)
{
}

//------------------------------------------------------------------------------
faction::~faction()
{
}

//------------------------------------------------------------------------------
void faction::think()
{
}

} // namespace game
