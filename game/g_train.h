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
    train(int num_cars);
    virtual ~train();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const;
    virtual void think() override;

    virtual vec2 get_position(time_value time) const override;
    virtual float get_rotation(time_value time) const override;
    virtual mat3 get_transform(time_value time) const override;

    void set_schedule(std::vector<handle<rail_station>> const& schedule);

    float length() const;

    void draw_debug(render::system* renderer, time_value time) const;
    void draw_path(render::system* renderer, time_value time) const;

    //! Update internal data when an edge in the rail network is split
    void on_edge_split(edge_index edge, edge_index new_edge, node_index new_node);

protected:
    std::vector<handle<rail_station>> _schedule;
    std::size_t _next_station;

    std::vector<edge_index> _path;
    float _current_distance;
    float _current_speed;
    float _current_acceleration;

    float _target_distance;

    int _num_cars;

    time_value _wait_time;
    time_value _idle_time; //!< time train has been idle

    enum class state {
        moving,
        waiting,
    } _state;

    mutable int _collision_type;
    mutable handle<train> _collision_train;

    struct debug {
        static constexpr int size = 1024;
        float current_distance[size];
        float current_speed[size];
        float current_acceleration[size];

        float target_distance[size];
        float collision_distance[size];
        int collision_type[size];
        edge_index collision_edge[size];
        int collision_train[size];
    } _debug;

    //! Maximum speed, ignoring track geometry
    static constexpr float max_speed = 50.f;
    //! Maximum acceleration
    static constexpr float max_acceleration = 1.f;
    //! Maximum deceleration
    static constexpr float max_deceleration = 1.f;
    //! Maximum lateral acceleration, used to determine maximum speed due to track curvature
    static constexpr float max_lateral_acceleration = 4.f;

    static constexpr float locomotive_length = 24.f;
    static constexpr float car_length = 16.f;
    static constexpr float coupling_length = 1.f;

    //! Minimum following distance
    static constexpr float tail_clearance = 15.f;

    static constexpr float distance_epsilon = 1e-1f;

protected:
    void next_station();

    float target_speed() const;
    float car_offset(int index) const;
    float truck_offset(int index) const;

    void draw(render::system* renderer, float distance, color4 color) const;
    void draw_locomotive(render::system* renderer, mat3 transform, color4 color) const;
    void draw_boxcar(render::system* renderer, mat3 transform, color4 color) const;
    void draw_tanker(render::system* renderer, mat3 transform, color4 color) const;
    void draw_coupler(render::system* renderer, mat3 transform, color4 color) const;

    //! Information about a path intersection between two trains
    struct intersection_info {
        //! First edge of the shared path
        edge_index edge;
        //! Length of the shared path
        float length;
        //! Distance to the first edge relative to the path of `this` and `other` respectively.
        float distance[2];
        //! Distance from the first edge needed to maintain lateral clearance for `this` and `other`.
        float enter_clearance[2];
    };

    bool check_collisions(float& collision_distance) const;
    bool check_collision(train const* other, float& collision_distance) const;
    bool check_path_intersection(train const* other, intersection_info& info) const;
};

} // namespace game
