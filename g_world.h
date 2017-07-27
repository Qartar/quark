// g_world.h
//

#pragma once

#include "g_usercmd.h"

#include "p_material.h"
#include "p_rigidbody.h"
#include "p_shape.h"

#include "r_model.h"
#include "r_particle.h"

#include <array>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

namespace physics {
struct contact;
} // namespace physics

namespace render {
class system;
} // namespace render

#define MAX_PLAYERS 16

////////////////////////////////////////////////////////////////////////////////
namespace game {

class tank;
class world;

typedef struct game_client_s game_client_t;

//------------------------------------------------------------------------------
enum class object_type
{
    object,
    obstacle,
    projectile,
    tank,
};

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
class object
{
public:
    object(object_type type, object* owner = nullptr);
    virtual ~object() {}

    std::size_t spawn_id() const { return _spawn_id; }

    virtual void draw(render::system* renderer, float time) const;
    virtual void touch(object *other, physics::contact const* contact);
    virtual void think();

    virtual void read_snapshot(network::delta_message const& message);
    virtual void write_snapshot(network::delta_message& message) const;

    //! Get frame-interpolated position
    virtual vec2 get_position(float lerp) const;

    //! Get frame-interpolated rotation
    virtual float get_rotation(float lerp) const;

    physics::rigid_body const& rigid_body() const { return _rigid_body; }

    void set_position(vec2 position, bool teleport = false);
    void set_rotation(float rotation, bool teleport = false);
    void set_linear_velocity(vec2 linear_velocity) { _rigid_body.set_linear_velocity(linear_velocity); }
    void set_angular_velocity(float angular_velocity) { _rigid_body.set_angular_velocity(angular_velocity); }

    vec2 get_position() const { return _rigid_body.get_position(); }
    float get_rotation() const { return _rigid_body.get_rotation(); }
    vec2 get_linear_velocity() const { return _rigid_body.get_linear_velocity(); }
    float get_angular_velocity() const { return _rigid_body.get_angular_velocity(); }

    void apply_impulse(vec2 impulse) { _rigid_body.apply_impulse(impulse); }
    void apply_impulse(vec2 impulse, vec2 position) { _rigid_body.apply_impulse(impulse, position); }

    render::model const* _model;
    color4 _color;

    vec2 _old_position;
    float _old_rotation;

    object_type _type;

protected:
    friend world;

    //! Game world which contains this object
    world* _world;

    object* _owner;

    std::size_t _spawn_id;

    physics::rigid_body _rigid_body;

    static physics::material _default_material;
    static physics::circle_shape _default_shape;
    constexpr static float _default_mass = 1.0f;
};

//------------------------------------------------------------------------------
class obstacle : public object
{
public:
    obstacle(physics::rigid_body&& rigid_body)
        : object(object_type::obstacle)
    {
        _rigid_body = std::move(rigid_body);
    }
};

//------------------------------------------------------------------------------
enum class weapon_type
{
    cannon,
    missile,
    blaster,
};

//------------------------------------------------------------------------------
class projectile : public object
{
public:
    projectile(tank* owner, float damage, weapon_type type);
    ~projectile();

    virtual void draw(render::system* renderer, float time) const override;
    virtual void touch(object *other, physics::contact const* contact) override;
    virtual void think() override;

    virtual void read_snapshot(network::delta_message const& message) override;
    virtual void write_snapshot(network::delta_message& message) const override;

    float damage() const { return _damage; }

    static physics::circle_shape _shape;
    static physics::material _material;

protected:
    float _damage;

    weapon_type _type;

    sound::channel* _channel;

    sound::asset _sound_cannon_impact;
    sound::asset _sound_blaster_impact;
    sound::asset _sound_missile_flight;

protected:
    void update_homing();
    void update_effects();
    void update_sound();
};

//------------------------------------------------------------------------------
class tank : public object
{
public:
    tank();
    ~tank();

    virtual void draw(render::system* renderer, float time) const override;
    virtual void touch(object *other, physics::contact const* contact) override;
    virtual void think() override;

    virtual void read_snapshot(network::delta_message const& message) override;
    virtual void write_snapshot(network::delta_message& message) const override;

    //! Get frame-interpolated turret rotation
    float get_turret_rotation(float lerp) const;

    void set_turret_rotation(float rotation, bool teleport = false);
    void set_turret_velocity(float velocity) { _turret_velocity = velocity; }
    float get_turret_rotation() const { return _turret_rotation; }
    float get_turret_velocity() const { return _turret_velocity; }

    void respawn();

    void update_usercmd(game::usercmd usercmd);

    void update_sound();

    char const* player_name() const;

    render::model const* _turret_model;
    float _turret_rotation;
    float _turret_velocity;
    float _old_turret_rotation;

    float _track_speed;

    float _damage;
    int _player_index;

    float _dead_time;
    float _fire_time;

    game::usercmd _usercmd;

    std::array<sound::channel*,3> _channels;
    game_client_t* _client;

    weapon_type _weapon;

protected:
    void collide(tank* other, physics::contact const* contact);

    void launch_projectile();

    void update_effects();

protected:
    static physics::material _material;
    static physics::box_shape _shape;

    constexpr static float cannon_speed = 1920.0f;
    constexpr static float missile_speed = 288.0f;
    constexpr static float blaster_speed = 768.0f;

    constexpr static float cannon_reload = 3.0f;
    constexpr static float missile_reload = 6.0f;
    constexpr static float blaster_reload = 0.3f;

    sound::asset _sound_idle;
    sound::asset _sound_move;
    sound::asset _sound_turret_move;
    sound::asset _sound_explode;
    sound::asset _sound_blaster_fire;
    sound::asset _sound_cannon_fire;
};

//------------------------------------------------------------------------------
class world
{
public:
    world ()
        : _spawn_id(0)
        , _players{}
        , _border_material{0,0}
        , _border_shapes{{vec2(0,0)}, {vec2(0,0)}}
        , _arena_width("g_arenaWidth", 640, config::archive|config::server|config::reset, "arena width")
        , _arena_height("g_arenaHeight", 480, config::archive|config::server|config::reset, "arena height")
    {}
    ~world () {}

    void init();
    void shutdown();
    void reset();

    void clear_particles();

    void run_frame ();
    void draw(render::system* renderer, float time) const;

    void read_snapshot(network::message& message);
    void write_snapshot(network::message& message) const;

    std::vector<std::unique_ptr<object>> const& objects() { return _objects; }

    template<typename T, typename... Args>
    T* spawn(Args&& ...args);

    game::object* find_object(std::size_t spawn_id) const;

    void remove(object* object);

    game::tank* spawn_player(int player_index);
    void remove_player(int player_index);

    void add_sound(sound::asset sound_asset, vec2 position, float volume = 1.0f);
    void add_effect(effect_type type, vec2 position, vec2 direction = vec2(0,0), float strength = 1);
    void add_trail_effect(effect_type type, vec2 position, vec2 old_position, vec2 direction = vec2(0,0), float strength = 1);

    vec2 mins() const { return _mins; }
    vec2 maxs() const { return _maxs; }
    int framenum() const { return _framenum; }

    game::tank* player( int index ) { return _players[ index ]; }

private:
    //! Active objects in the world
    std::vector<std::unique_ptr<object>> _objects;

    //! Objects pending addition
    std::vector<std::unique_ptr<object>> _pending;

    //! Objects pending removal
    std::set<object*> _removed;

    //! Previous object spawn id
    std::size_t _spawn_id;

    void move_object(object* object);

    game::object* spawn_snapshot(std::size_t spawn_id, object_type type);

    config::integer _arena_width;
    config::integer _arena_height;

    friend game::tank;

    game::tank* _players[MAX_PLAYERS];

    //
    // particle system
    //

    bool _use_particles;

    mutable std::vector<render::particle> _particles;

    render::particle* add_particle();
    void free_particle (render::particle* particle) const;

    void draw_particles(render::system* renderer, float time) const;

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
    void write_effect(effect_type type, vec2 position, vec2 direction, float strength);
};

//------------------------------------------------------------------------------
template<typename T, typename... Args>
T* world::spawn(Args&& ...args)
{
    static_assert(std::is_base_of<game::object, T>::value,
                  "'spawn': 'T' must be derived from 'game::object'");

    _pending.push_back(std::make_unique<T>(std::move(args)...));
    _pending.back()->_world = this;
    _pending.back()->_spawn_id = ++_spawn_id;
    return static_cast<T*>(_pending.back().get());
}

} // namespace game
