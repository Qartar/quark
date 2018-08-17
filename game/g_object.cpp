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
    : _model(nullptr)
    , _color(1,1,1,1)
    , _old_position(vec2_zero)
    , _old_rotation(0)
    , _type(type)
    , _owner(owner)
    , _rigid_body(&_default_shape, &_default_material, _default_mass)
{}

//------------------------------------------------------------------------------
void object::spawn()
{
    _random = random(get_world()->get_random());
}

//------------------------------------------------------------------------------
bool object::touch(object* /*other*/, physics::collision const* /*collision*/)
{
    return true;
}

//------------------------------------------------------------------------------
void object::draw(render::system* renderer, time_value time) const
{
    if (_model) {
        renderer->draw_model(_model, get_transform(time), _color);
    }
}

//------------------------------------------------------------------------------
void object::think()
{
}

//------------------------------------------------------------------------------
void object::read_snapshot(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void object::write_snapshot(network::message& /*message*/) const
{
}

//------------------------------------------------------------------------------
vec2 object::get_position(time_value time) const
{
    float lerp = (time - get_world()->frametime()) / FRAMETIME;
    return _old_position + (get_position() - _old_position) * lerp;
}

//------------------------------------------------------------------------------
float object::get_rotation(time_value time) const
{
    float lerp = (time - get_world()->frametime()) / FRAMETIME;
    return _old_rotation + (get_rotation() - _old_rotation) * lerp;
}

//------------------------------------------------------------------------------
mat3 object::get_transform(time_value time) const
{
    return mat3::transform(get_position(time), get_rotation(time));
}

//------------------------------------------------------------------------------
mat3 object::get_inverse_transform(time_value time) const
{
    return mat3::inverse_transform(get_position(time), get_rotation(time));
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
