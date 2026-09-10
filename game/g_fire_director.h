// g_fire_director.h
//

#pragma once

#include "g_subsystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class ship;
struct gun_design;

//------------------------------------------------------------------------------
class fire_director : public subsystem
{
public:
    static const object_type _type;

public:
    fire_director(game::ship* owner, gun_design const* gun);

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual void think() override;

    void set_target(handle<ship const> target);
    handle<ship const> get_target() const { return _target; }

    //! Returns true if the fire director has a valid firing solution
    bool has_solution() const { return _is_valid; }
    //! Return the current firing solution
    void get_solution(double& range, double& bearing, double& elevation) const;

    //! Update aim correction based on the shell splash location
    bool splash_observation(handle<ship const> target, float shell_size, vec3 splash_origin);

protected:
    handle<ship const> _target;
    gun_design const* _gun;

    double _range;
    double _bearing;
    double _elevation;
    time_delta _time_of_flight;
    bool _is_valid;

    static constexpr std::size_t max_corrections = 16;

    //! Correction data to account for variables not reflected in the range
    //! table, e.g. wind, temperature, humidity, barrel wear, etc.
    struct correction {
        time_value time; //!< Time of this splash/correction
        vec3 offset; //!< Offset for this specific splash/correction
        vec3 total; //!< Total correction, i.e. running average
    } _corrections[max_corrections];

    std::size_t _num_corrections;

    static config::boolean _show_correction;

protected:
    void update_solution();
};

} // namespace game
