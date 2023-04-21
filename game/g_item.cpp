// g_item.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_item.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type item::_type(object::_type);

//------------------------------------------------------------------------------
item::item(definition const* def)
    : _definition(def)
{
    traits t0{};
    traits t1(trait::edible);
    traits t2(trait::edible, trait::burnable);
    traits t3(trait::edible, trait::burnable, trait::foragable);
}

//------------------------------------------------------------------------------
item::~item()
{
}

//------------------------------------------------------------------------------
void item::draw(render::system* renderer, time_value time) const
{
    if (_definition) {
        renderer->draw_arc(
            get_position(time),
            _definition->radius,
            0.f,
            0.f,
            2.f * math::pi<float>,
            _definition->color);
    }
}

} // namespace game
