// g_resource.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_resource.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type resource::_type(object::_type);

//------------------------------------------------------------------------------
resource::resource(definition const* def)
    : _definition(def)
{
}

//------------------------------------------------------------------------------
resource::~resource()
{
}

//------------------------------------------------------------------------------
void resource::draw(render::system* renderer, time_value time) const
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
