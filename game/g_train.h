// g_train.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class rail_network;
class rail_station;

//------------------------------------------------------------------------------
class train : public object
{
public:
    static const object_type _type;

    using node_index = int;
    using edge_index = int;

public:
    train();
    virtual ~train();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const;
    virtual void think() override;

    virtual vec2 get_position(time_value time) const override;
    virtual float get_rotation(time_value time) const override;
    virtual mat3 get_transform(time_value time) const override;

protected:
    std::vector<handle<rail_station>> _schedule;
    std::size_t _next_station;

    std::vector<edge_index> _path;
    edge_index _current_edge;
    float _current_distance;
    float _current_speed;

    std::vector<edge_index> _prev;
    int _num_cars;

    static constexpr float max_speed = 50.f;
    static constexpr float max_acceleration = 4.f;
    static constexpr float max_deceleration = 4.f;
    static constexpr float max_lateral_acceleration = 4.f;

protected:
    float target_speed() const;
    float car_offset(int index) const;
};

} // namespace game
