// g_object.cpp
//

#include "precompiled.h"
#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////
namespace game {

std::array<object_type*, object_type::_max_types> object_type::_types;
std::size_t object_type::_num_types = 1; // type index 0 is the 'null' type

//------------------------------------------------------------------------------
object_type::object_type()
    : _type_index(_num_types)
    , _num_derived(0)
    /*
        Note: `_link` and `_next` must NOT be initialized by the constructor for
        initialization to work correctly since they are modified by any derived
        type that is initialized before the base type. This technique depends on
        zero-initialization of static storage which is guaranteed by the C++ standard.
    */
{
    _types[_num_types++] = this;
    // link derived types that were initialized before this type
    for (auto* t = _link; t; t = t->_next) {
        t->link(*this);
    }
    _link = nullptr;
}

//------------------------------------------------------------------------------
object_type::object_type(object_type const& base)
    : object_type()
{
    link(base);
}

//------------------------------------------------------------------------------
void object_type::link(object_type const& base)
{
    // if the base type has been initialized
    if (base._type_index) {
        // link derived types that were initialized before this type
        for (auto* t = _link; t; t = t->_next) {
            t->link(*this);
        }
        _link = nullptr;
        // insert this type into the type hierarchy
        insert(_type_index, base._type_index);
    } else {
        // add this type into the base type's deferred link list
        _next = base._link;
        const_cast<object_type&>(base)._link = this;
    }
}

//------------------------------------------------------------------------------
void object_type::insert(std::size_t type_index, std::size_t base_index)
{
    std::size_t first = 0, n_first = 0, last = 0;
    //! index of the first type that is not derived from base type
    std::size_t last_index = base_index + _types[base_index]->_num_derived + 1;

    // add derived types to all ancestors of the type, starting with base type
    for (std::size_t ii = base_index; ii > 0; --ii) {
        if (ii + _types[ii]->_num_derived >= base_index) {
            _types[ii]->_num_derived += 1 + _types[type_index]->_num_derived;
        }
    }

    if (type_index < base_index) {
        // Rotate base type and base-derived types in front of this type
        //   [this[derived]][...][base[derived]]
        //                        ^~~~~~~~~~~~~
        //   [base[derived][this[derived]]][...]
        //    ^~~~~~~~~~~~~
        first = type_index;
        n_first = base_index;
        last = last_index;
    } else if (type_index > last_index) {
        // Rotate this type and this-derived types to the end of base-derived types
        //   [base[derived]][...][this[derived]]
        //                        ^~~~~~~~~~~~~
        //   [base[derived][this[derived]]][...]
        //                  ^~~~~~~~~~~~~
        first = last_index;
        n_first = type_index;
        last = type_index + _types[type_index]->_num_derived + 1;
    } else {
        // If type is not at last_index it means it is somewhere inside the list
        // of derived types of the base type, which means the derived types were
        // not contiguous, which means the algorithm is broken.
        assert(type_index == last_index);
    }
    // rotate types into place
    std::rotate(_types.begin() + first,
                _types.begin() + n_first,
                _types.begin() + last);
    // update type indices
    for (std::size_t ii = first; ii < last; ++ii) {
        _types[ii]->_type_index = ii;
    }
}

//------------------------------------------------------------------------------
const object_type object::_type;
physics::material object::_default_material(0.5f, 0.5f);
physics::circle_shape object::_default_shape(0.5f);

//------------------------------------------------------------------------------
object::object(object* owner)
    : _model(nullptr)
    , _color(1,1,1,1)
    , _old_position(vec2_zero)
    , _old_rotation(0)
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
void object::read_snapshot(network::delta_message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void object::write_snapshot(network::delta_message& /*message*/) const
{
}

//------------------------------------------------------------------------------
vec2 object::get_position(time_value time) const
{
    float lerp = (time - get_world()->frametime()) / FRAMETIME;
    return _old_position + (get_position() - _old_position) * lerp;
}

//------------------------------------------------------------------------------
rot2 object::get_rotation(time_value time) const
{
    float lerp = (time - get_world()->frametime()) / FRAMETIME;
    return _old_rotation * rot2((get_rotation() / _old_rotation).radians() * lerp);
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
void object::set_rotation(rot2 rotation, bool teleport/* = false*/)
{
    _rigid_body.set_rotation(rotation);
    if (teleport) {
        _old_rotation = rotation;
    }
}

} // namespace game
