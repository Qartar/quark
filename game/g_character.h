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

protected:
    string::buffer _name;

    // needs
    float _hungry;
    float _thirsty;
    float _drowsy;
    float _tired;
    float _cold;
    float _hot;
    float _wet;

    // non-environmental tick rates
    float _hunger_per_tick;
    float _thirst_per_tick;
    float _drowsy_per_tick;
    float _tired_per_tick;
    float _resting_drowsy_per_tick;
    float _resting_tired_per_tick;

protected:
    //! recalculate tick rates for non-environmental needs
    void recalculate_rates();

    void update_needs();
    void update_health();
    void update_tasks();
    void update_planning();
    void update_movement();
    void update_action();
};

} // namespace game
