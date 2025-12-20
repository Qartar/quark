// g_faction.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
class faction : public object
{
public:
    static const object_type _type;

public:
    faction(string::view name, color4 color);
    virtual ~faction();

    virtual object_type const& type() const override { return _type; }
    virtual void think() override;

    string::view name() const { return _name; }
    color4 color() const { return _color; }

protected:
    string::buffer _name;
    color4 _color;
};

} // namespace game
