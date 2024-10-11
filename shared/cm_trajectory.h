// cm_trajectory.h
//

#pragma once

#include "cm_time.h"
#include "cm_vector.h"

////////////////////////////////////////////////////////////////////////////////
/**
 * Universal variable formulation of an object's trajectory inside of a two-body
 * problem. The trajectory has an arbitrary orientation but it is originated
 * about the center of mass of the system.
 */
class trajectory
{
public:
    trajectory(float mu, time_value t0, vec2 r0, vec2 v0);

    float mu() const { return _mu; }
    float characteristic_energy() const { return -_alpha; }

    time_value initial_epoch() const { return _t0; }
    vec2 initial_position() const { return _r0; }
    vec2 initial_velocity() const { return _v0; }
    float initial_distance() const { return _r0norm; }

    //! Calculate the state vectors `r` and `v` at the given time `t`
    void calculate(time_value t, vec2* r, vec2* v) const;

    //! Calculate the state vectors `r` and `v` directly from `s`
    void step(float s, vec2* r, vec2* v) const;

protected:
    float _mu; //!< standard gravitational parameter
    time_value _t0; //!< initial epoch
    vec2 _r0; //!< initial displacement vector
    vec2 _v0; //!< initial velocity vector

    float _r0norm; //!< magnitude of initial displacement
    float _alpha; //!< negative characteristic energy

protected:
    void calculate_s(time_value t, float* s_ptr, float* s2_ptr, float* c) const;
    time_value calculate_t(float s, float s2, float const* c) const;
    float calculate_dtds(float s, float s2, float const* c, float const* dcds) const;
};
