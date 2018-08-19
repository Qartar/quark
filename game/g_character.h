// g_character.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class subsystem;

//------------------------------------------------------------------------------
class character : public object
{
public:
    character();
    virtual ~character();

    virtual void think() override;

    std::string const& name() const { return _name; }
    void damage(object* inflictor, float amount);
    float health() const { return _health; }

    void assign(handle<subsystem> assignment) { _subsystem = assignment; }
    handle<subsystem> assignment() const { return _subsystem; }

protected:
    std::string _name;
    float _health;

    handle<subsystem> _subsystem;

    static constexpr float repair_rate = 1.f / 5.f; //!< damage per second
};

} // namespace game
