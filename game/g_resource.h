// g_resource.h
//

#pragma once

#include "g_object.h"
#include "g_traits.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
class resource : public object
{
public:
    static const object_type _type;

    struct definition
    {
        string::buffer name;
        traits traits;
        float radius;
        color4 color;
    };

public:
    resource(definition const* def);
    virtual ~resource();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const;

protected:
    definition const* _definition;
};

} // namespace game
