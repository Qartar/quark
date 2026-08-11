// g_navigation.cpp
//

#pragma once

#include "g_subsystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class formation;

//------------------------------------------------------------------------------
class navigation : public subsystem
{
public:
    static const object_type _type;

public:
    navigation(game::ship* owner);
    ~navigation();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    void set_speed(float speed) { _target_speed = speed; }

    void set_heading(rot2 heading) {
        _target_heading = heading;
        _waypoints.resize(0);
    }

    std::vector<vec3> const& waypoints() const { return _waypoints; }

    void set_waypoint(vec3 point) {
        _waypoints.resize(0);
        _waypoints.push_back(point);
    }
    void add_waypoint(vec3 point) {
        _waypoints.push_back(point);
    }

    void set_formation(handle<formation> f, std::size_t idx) {
        _formation = f;
        _formation_index = idx;
    }

protected:
    float _target_speed;
    rot2 _target_heading;
    std::vector<vec3> _waypoints;

    handle<formation> _formation;
    std::size_t _formation_index;

    static config::boolean _show_navigation;
};

} // namespace game
