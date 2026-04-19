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
    virtual void think() override;

    void set_target(handle<ship const> target);
    handle<ship const> get_target() const { return _target; }

    //! Returns true if the fire director has a valid firing solution
    bool has_solution() const { return _is_valid; }
    //! Return the current firing solution
    void get_solution(float& bearing, float& elevation) const;

protected:
    handle<ship const> _target;
    gun_design const* _gun;

    float _bearing;
    float _elevation;
    time_delta _time_of_flight;
    bool _is_valid;

    // TODO: Move range table to gun design instead of duplicating it for every
    // fire director (e.g. every ship) with that gun design.
    struct range
    {
        float elevation;
        float range;
        time_delta time_of_flight;
    };

    static constexpr std::size_t max_table = 128;
    range _table[max_table];
    std::size_t _table_size;

    static constexpr float min_elevation = math::deg2rad(0.f);
    static constexpr float max_elevation = math::deg2rad(50.f);

protected:
    void update_solution();

    //! Fill range data by linear interpolation of the range table. If the given
    //! range falls outside the bounds of the range table then use the nearest
    //! row of the table and return false.
    bool interpolate_range(float d, range& r) const;

    //! Populate the range table by simulating the projectile at varying elevations.
    void populate_table();
};

} // namespace game
