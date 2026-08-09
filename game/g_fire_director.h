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
    void get_solution(double& bearing, double& elevation) const;

    //! Update aim correction based on the shell splash location
    bool splash_observation(handle<ship const> target, float shell_size, vec3 splash_origin);

protected:
    handle<ship const> _target;
    gun_design const* _gun;

    double _bearing;
    double _elevation;
    time_delta _time_of_flight;
    bool _is_valid;

    // TODO: Move range table to gun design instead of duplicating it for every
    // fire director (e.g. every ship) with that gun design.
    struct range
    {
        double elevation;
        double range;
        time_delta time_of_flight;
    };

    static constexpr std::size_t max_table = 128;
    range _table[max_table];
    std::size_t _table_size;

    static constexpr double min_elevation = math::deg2rad(0.0);
    static constexpr double max_elevation = math::deg2rad(50.0);

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

    //! Fill range data by linear interpolation of the range table. If the given
    //! range falls outside the bounds of the range table then use the nearest
    //! row of the table and return false.
    bool interpolate_range(double d, range& r) const;

    //! Populate the range table by simulating the projectile at varying elevations.
    void populate_table();
};

} // namespace game
