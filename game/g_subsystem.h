// g_subsystem.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class ship;

//------------------------------------------------------------------------------
class subsystem : public object
{
public:
    static const object_type _type;

public:
    subsystem(game::ship* owner);

    virtual object_type const& type() const override { return _type; }
    virtual void think() override;

    void damage(object* inflictor, float amount);
    float damage() const { return _damage; }
    void repair(float damage_per_second);

protected:
    float _damage;

    time_value _damage_time;
    static constexpr time_delta repair_delay = time_delta::from_seconds(0.5f);
};

//------------------------------------------------------------------------------
class engines : public subsystem
{
public:
    static const object_type _type;

public:
    engines(game::ship* owner);

    virtual object_type const& type() const override { return _type; }
    virtual void think() override;

    void set_rudder_target(float rudder_target) { _rudder_target = rudder_target; }
    float get_rudder_target() const { return _rudder_target; }
    float get_rudder_angle() const { return _rudder_angle; }

    void set_speed_target(float speed_target) { _speed_target = speed_target; }
    float get_speed_target() const { return _speed_target; }

protected:
    //! Linear drag coefficients along longitudinal and transverse axes. These values
    //! also include density and cross-sectional area terms since they are constant.
    float _linear_drag_coefficient[2];
    //! Torque per angular velocity squared due to drag
    float _angular_drag_coefficient;
    //! Inverse moment of inertia
    float _inverse_inertia;

    float _rudder_angle;
    float _rudder_target;

    float _speed_target;
};

} // namespace game
