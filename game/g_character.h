// g_character.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class ship;
class subsystem;
class ship_layout;

//------------------------------------------------------------------------------
class character : public object
{
public:
    static const object_type _type;

public:
    character();
    virtual ~character();

    virtual object_type const& type() const override { return _type; }
    virtual void think() override;

    string::view name() const { return _name; }
    void damage(object* inflictor, float amount);
    float health() const { return _health; }

    using object::set_position;
    void set_position(handle<ship> ship, vec2 position, bool teleport = false);

    void assign(handle<subsystem> assignment) { _subsystem = assignment; }
    handle<subsystem> assignment() const { return _subsystem; }

    void set_goal(vec2 goal);

protected:
    string::buffer _name;
    float _health;

    static constexpr int path_size = 32;
    vec2 _path[path_size];
    int _path_start;
    int _path_end;

    handle<ship> _ship;

    handle<subsystem> _subsystem;

    static constexpr float repair_rate = 1.f / 5.f; //!< damage per second
};

} // namespace game
