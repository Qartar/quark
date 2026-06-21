// g_formation.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class ship;

//------------------------------------------------------------------------------
class formation : public object
{
public:
    static const object_type _type;

    //! Maximum number of objects in a formation
    static constexpr std::size_t maximum_size = 8;

public:
    formation();
    ~formation();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual void think() override;

    //! Set the formation objects, any objects currently in the formation will be removed
    void set(handle<object> const* objects, std::size_t size);
    //! Add the given objects to the formation
    void add(handle<object> obj);
    //! Remove the given object from the formation
    void remove(handle<object> obj);

    template<std::size_t sz> void set(handle<object> const (&objects)[sz]) {
        static_assert(sz <= maximum_size);
        set(objects, sz);
    }

    //! Number of objects in the formation
    std::size_t size() const { return _size; }
    //! Maximum speed for all ships in the formation
    double maximum_speed() const { return _maximum_speed; }
    //! Current target speed
    double target_speed() const { return _target_speed; }
    //! Current formation target position for the given object index
    vec3 target_position(std::size_t index) const {
        assert(index < _size);
        return _target_position[index];
    }
    //! Current formation target velocity for the given object index
    vec3 target_velocity(std::size_t index) const {
        assert(index < _size);
        return _target_velocity[index];
    }

protected:
    std::size_t _size; //!< Number of objects in the formation
    handle<object> _objects[maximum_size]; //!< Objects in the formation
    double _spacing; //!< Desired spacing between objects

    double _maximum_speed; //!< Maximum speed for all ships in the formation
    double _target_speed; //!< Current target speed

    vec3 _target_position[maximum_size];
    vec3 _target_velocity[maximum_size];
};

} // namespace game
