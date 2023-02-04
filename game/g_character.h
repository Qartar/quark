// g_character.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

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

protected:
    string::buffer _name;
    float _health;
};

} // namespace game
