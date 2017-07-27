// g_object.cpp
//

#include "precompiled.h"
#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////
namespace game {

physics::material object::_default_material(0.5f, 0.5f);
physics::circle_shape object::_default_shape(0.5f);

//------------------------------------------------------------------------------
object::object(object_type type, object* owner)
    : _world(nullptr)
    , _model(nullptr)
    , _color(1,1,1,1)
    , _type(type)
    , _owner(owner)
    , _rigid_body(&_default_shape, &_default_material, _default_mass)
{}

//------------------------------------------------------------------------------
void object::touch(object* /*other*/, physics::contact const* /*contact*/)
{
}

//------------------------------------------------------------------------------
void object::draw(render::system* /*renderer*/, float /*time*/) const
{
    if (_model) {
        _model->draw(get_position(), get_rotation(), _color);
    }
}

//------------------------------------------------------------------------------
void object::think()
{
}

//------------------------------------------------------------------------------
void object::read_snapshot(network::delta_message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void object::write_snapshot(network::delta_message& /*message*/) const
{
}

//------------------------------------------------------------------------------
vec2 object::get_position(float lerp) const
{
    return _old_position + (get_position() - _old_position) * lerp;
}

//------------------------------------------------------------------------------
float object::get_rotation(float lerp) const
{
    return _old_rotation + (get_rotation() - _old_rotation) * lerp;
}

//------------------------------------------------------------------------------
void object::set_position(vec2 position, bool teleport/* = false*/)
{
    _rigid_body.set_position(position);
    if (teleport) {
        _old_position = position;
    }
}

//------------------------------------------------------------------------------
void object::set_rotation(float rotation, bool teleport/* = false*/)
{
    _rigid_body.set_rotation(rotation);
    if (teleport) {
        _old_rotation =  rotation;
    }
}

} // namespace game
