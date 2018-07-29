// g_world.h
//

#pragma once

#include "g_usercmd.h"
#include "g_object.h"

#include "p_material.h"
#include "p_rigidbody.h"
#include "p_shape.h"
#include "p_world.h"

#include "net_message.h"

#include "r_model.h"
#include "r_particle.h"

#include <array>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

#define MAX_PLAYERS 16

constexpr const time_delta FRAMETIME = time_delta::from_seconds(0.05f);

////////////////////////////////////////////////////////////////////////////////
namespace game {

class object;
class world;

typedef struct game_client_s game_client_t;

//------------------------------------------------------------------------------
enum class effect_type
{
    smoke,
    sparks,
    blaster,
    missile_trail,
    cannon_impact,
    missile_impact,
    blaster_impact,
    explosion,
};

//------------------------------------------------------------------------------
class world
{
public:
    world ();
    ~world () {}

    void init();
    void shutdown();
    void reset();

    void clear_particles();

    void run_frame ();
    void draw(render::system* renderer, time_value time) const;

    void read_snapshot(network::message& message);
    void write_snapshot(network::message& message) const;

    std::vector<std::unique_ptr<object>> const& objects() { return _objects; }

    template<typename T, typename... Args>
    T* spawn(Args&& ...args);

    game::object* find_object(std::size_t spawn_id) const;

    random& get_random() { return _random; }

    void remove(object* object);

    void add_sound(sound::asset sound_asset, vec2 position, float volume = 1.0f);
    void add_effect(time_value time, effect_type type, vec2 position, vec2 direction = vec2(0,0), float strength = 1);
    void add_trail_effect(effect_type type, vec2 position, vec2 old_position, vec2 direction = vec2(0,0), float strength = 1);

    void add_body(game::object* owner, physics::rigid_body* body);
    void remove_body(physics::rigid_body* body);

    game::object* trace(physics::contact& contact, vec2 start, vec2 end, game::object const* ignore = nullptr) const;

    vec2 mins() const { return _mins; }
    vec2 maxs() const { return _maxs; }
    int framenum() const { return _framenum; }
    time_value frametime() const { return time_value(_framenum * FRAMETIME); }

private:
    //! Active objects in the world
    std::vector<std::unique_ptr<object>> _objects;

    //! Objects pending addition
    std::vector<std::unique_ptr<object>> _pending;

    //! Objects pending removal
    std::set<object*> _removed;

    //! Previous object spawn id
    std::size_t _spawn_id;

    physics::world _physics;
    std::map<physics::rigid_body const*, game::object*> _physics_objects;

    //! Random number generator
    random _random;

    bool physics_filter_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b);
    bool physics_collide_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b, physics::collision const& collision);

    game::object* spawn_snapshot(std::size_t spawn_id, object_type type);

    config::integer _arena_width;
    config::integer _arena_height;

    //
    // particle system
    //

    mutable std::vector<render::particle> _particles;

    render::particle* add_particle(time_value time);
    void free_particle (render::particle* particle) const;

    void draw_particles(render::system* renderer, time_value time) const;

    vec2        _mins;
    vec2        _maxs;

    int _framenum;

    network::message_storage _message;

    physics::material _border_material;
    physics::box_shape _border_shapes[2];
    constexpr static int _border_thickness = 512;

protected:
    enum class message_type
    {
        none,
        frame,
        sound,
        effect,
    };

    void read_frame(network::message const& message);
    void read_sound(network::message const& message);
    void read_effect(network::message const& message);

    void write_sound(sound::asset sound_asset, vec2 position, float volume);
    void write_effect(time_value time, effect_type type, vec2 position, vec2 direction, float strength);
};

//------------------------------------------------------------------------------
template<typename T, typename... Args>
T* world::spawn(Args&& ...args)
{
    static_assert(std::is_base_of<game::object, T>::value,
                  "'spawn': 'T' must be derived from 'game::object'");

    _pending.push_back(std::make_unique<T>(std::move(args)...));
    T* obj = static_cast<T*>(_pending.back().get());
    obj->_world = this;
    obj->_spawn_id = ++_spawn_id;
    obj->_spawn_time = frametime();
    obj->spawn();
    return obj;
}

} // namespace game
