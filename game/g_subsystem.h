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

    void damage(object* inflictor, double amount);
    double damage() const { return _damage; }
    void repair(double damage_per_second);

protected:
    double _damage;

    time_value _damage_time;
    static constexpr time_delta repair_delay = time_delta::from_seconds(0.5);
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

    void set_rudder_target(double rudder_target) { _rudder_target = rudder_target; }
    double get_rudder_target() const { return _rudder_target; }
    double get_rudder_angle() const { return _rudder_angle; }

    void set_speed_target(double speed_target) { _speed_target = speed_target; }
    double get_speed_target() const { return _speed_target; }

protected:
    //! Linear drag coefficients along longitudinal and transverse axes. These values
    //! also include density and cross-sectional area terms since they are constant.
    double _linear_drag_coefficient[2];
    //! Torque per angular velocity squared due to drag
    double _angular_drag_coefficient;
    //! Inverse moment of inertia
    double _inverse_inertia;

    double _rudder_angle;
    double _rudder_target;

    double _speed_target;

    //! Ratio of total torque applied directly by the rudder. In ships the rudder
    //! imparts only a small amount of torque; most of the turning moment comes
    //! from unbalanced form drag caused by lateral motion (i.e. "sway") induced
    //! by the rudder. The simulation ignores the torque from form drag and applies
    //! the total turning torque as if it were entirely driven by the rudder but
    //! scales the linear force by this value, otherwise the rudder will induce an
    //! absurd amount of drag and lateral motion.
    static constexpr double rudder_torque_coefficient = 0.05;
};

} // namespace game
