// g_navigation.cpp
//

#pragma once

#include "g_subsystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

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

    std::vector<vec2> const& waypoints() const { return _waypoints; }

    void set_waypoint(vec2 point) {
        _waypoints.resize(0);
        _waypoints.push_back(point);
    }
    void add_waypoint(vec2 point) {
        _waypoints.push_back(point);
    }

protected:
    std::vector<vec2> _waypoints;
};

} // namespace game
