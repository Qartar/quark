// g_world.h
//

#pragma once

#include "g_usercmd.h"
#include "g_object.h"
#include "g_rail_network.h"

#include "p_material.h"
#include "p_rigidbody.h"
#include "p_shape.h"
#include "p_world.h"

#include "net_message.h"

#include "r_particle.h"

#include <array>
#include <memory>
#include <queue>
#include <type_traits>
#include <vector>

#define MAX_PLAYERS 16

constexpr const time_delta FRAMETIME = time_delta::from_seconds(0.05f);

////////////////////////////////////////////////////////////////////////////////
namespace game {

class object;
class world;

//------------------------------------------------------------------------------
enum class effect_type
{
    none,
    smoke,
    sparks,
    cannon,
    blaster,
    missile_trail,
    cannon_impact,
    missile_impact,
    blaster_impact,
    explosion,
};

//------------------------------------------------------------------------------
template<typename type> class object_iterator
{
public:
    //  required for input_iterator

    bool operator!=(object_iterator const& other) const {
        return _type_index != other._type_index || _type_offset != other._type_offset;
    }

    type* operator*() const {
        return reinterpret_cast<type*>(_objects_data[_type_index].data() + _type_offset);
    }

    object_iterator& operator++() {
        return next();
    }

    //  required for forward_iterator

    object_iterator operator++(int) const {
        return object_iterator(*this).next();
    }

protected:
    std::vector<std::vector<byte>>& _objects_data;
    std::size_t _type_index;
    std::size_t _last_index;
    std::size_t _type_offset;

protected:
    template<typename> friend class object_range;

    object_iterator(std::vector<std::vector<byte>>& objects_data, std::size_t type_index, std::size_t last_index)
        : _objects_data(objects_data)
        , _type_index(type_index)
        , _last_index(last_index)
        , _type_offset(0)
    {
        // advance to the first active object
        if (_type_index <= _last_index) {
            _type_offset -= object_type::type_size(_type_index);
            next();
        }
}

    object_iterator& next() {
        assert(_type_index <= _last_index);
        std::size_t type_size = object_type::type_size(_type_index);
        do {
            _type_offset += type_size;
            while (_type_offset >= _objects_data[_type_index].size()) {
                _type_offset = 0;
                if (++_type_index > _last_index) {
                    return *this;
                }
                type_size = object_type::type_size(_type_index);
            }
        } while (!reinterpret_cast<type*>(_objects_data[_type_index].data() + _type_offset)->get_sequence());
        return *this;
    }
};

//------------------------------------------------------------------------------
template<typename type> class object_range
{
public:
    using iterator_type = object_iterator<type>;

    iterator_type begin() const { return iterator_type(_objects_data, _type_index, _last_index); }
    iterator_type end() const { return iterator_type(_objects_data, _last_index + 1, _last_index); }

protected:
    friend world;

    std::vector<std::vector<byte>>& _objects_data;
    std::size_t _type_index;
    std::size_t _last_index;

protected:
    object_range(std::vector<std::vector<byte>>& objects_data)
        : _objects_data(objects_data)
        , _type_index(typename type::_type.index())
        , _last_index(typename type::_type.num_derived() + _type_index)
    {}
};

//------------------------------------------------------------------------------
class world
{
public:
    world ();
    ~world ();

    void init();
    void shutdown();

    //! Reset world to initial playable state
    void reset();
    //! Clear all allocated objects, particles, and internal data
    void clear();

    void clear_particles();

    void run_frame ();
    void draw(render::system* renderer, time_value time) const;

    void read_snapshot(network::message& message);
    void write_snapshot(network::message& message) const;

    template<typename T, typename... Args>
    T* spawn(Args&& ...args);

    //! Return an iterator over all active objects in the world
    template<typename T = object> object_range<T const> objects() const;

    //! Return an iterator over all active objects in the world
    template<typename T = object> object_range<T> objects();

    random& get_random() { return _random; }

    void remove(handle<object> object);

    void add_sound(sound::asset sound_asset, vec2 position, float volume = 1.0f);
    void add_effect(time_value time, effect_type type, vec2 position, vec2 direction = vec2(0,0), float strength = 1);
    void add_trail_effect(effect_type type, vec2 position, vec2 old_position, vec2 direction = vec2(0,0), float strength = 1);

    void add_body(game::object* owner, physics::rigid_body* body);
    void remove_body(physics::rigid_body* body);

    game::object* trace(physics::contact& contact, vec2 start, vec2 end, game::object const* ignore = nullptr) const;

    int framenum() const { return _framenum; }
    time_value frametime() const { return time_value(_framenum * FRAMETIME); }

    rail_network& rail_network() { return _rail_network; }

    float timescale() const { return _timescale; } //!< Game speed as a multiplier

    void on_speed_up(); //!< Command callback for increasing the game speed
    void on_speed_down(); //!< Command callback for decreasing the game speed
    void on_pause(); //!< Command callback for pausing/unpausing the game world

private:
    //! Opaque arrays of objects in the world
    std::vector<std::vector<byte>> _objects_data;

    //! Objects pending removal
    std::queue<handle<object>> _removed;

    //! World index in singletons array
    uint64_t _index;

    //! Sequence id of most recently spawned object
    uint64_t _sequence;

    //! Random number generator
    random _random;

    template<typename T> friend class handle;

    //! Maximum number of objects that can be referenced by handle
    constexpr static int max_objects = 1LLU << handle<object>::index_bits;
    //! Maximum number of worlds than can be referenced by handle
    constexpr static int max_worlds = 1LLU << handle<object>::system_bits;

    //! Static array of worlds so that handles can store an index instead of pointer
    static std::array<world*, max_worlds> _singletons;

    //! Retrieve an object from its handle
    template<typename T> T* get(handle<T> handle) const;

    physics::world _physics;
    std::map<physics::rigid_body const*, game::object*> _physics_objects;

    bool physics_filter_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b);
    bool physics_collide_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b, physics::collision const& collision);

    float _timescale; //!< Current game speed as a multiplier
    float _prev_timescale; //!< Previous game speed, used for unpausing

    //
    // particle system
    //

    mutable std::vector<render::particle> _particles;

    render::particle* add_particle(time_value time);
    void free_particle (render::particle* particle) const;

    void draw_particles(render::system* renderer, time_value time) const;

    int _framenum;

    network::message_storage _message;

    game::rail_network _rail_network;

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

    std::size_t type_index = T::_type.index();
    if (type_index >= _objects_data.size()) {
        _objects_data.resize(type_index + 1);
    }

    auto& type_data = _objects_data[type_index];
    std::size_t type_size = sizeof(T);

    // Check for an uninitialized object in existing objects data
    for (std::size_t ii = 0, sz = type_data.size(); ii < sz; ii += type_size) {
        T* obj = reinterpret_cast<T*>(type_data.data() + ii);
        // check if object is valid
        if (obj->_self.get_sequence() != 0) {
            continue; // TODO: skiplist
        }

        obj = new (type_data.data() + ii) T(std::move(args)...);
        obj->_self = handle<object>(type_index, ii / type_size, _index, ++_sequence);
        obj->_spawn_time = frametime();
        obj->spawn();
        return obj;
    }

    // Resize objects data
    std::size_t prev_size = type_data.size();
    if (std::is_pod<T>()) {
        type_data.resize(max(type_size * 8, prev_size * 2));
        memset(type_data.data() + prev_size, 0, type_data.size() - prev_size);
    } else {
        // Ideally all object types would be POD and memcpyable
        std::vector<byte> new_data(max(type_size * 8, prev_size * 2));
        for (std::size_t ii = 0, sz = type_data.size(); ii < sz; ii += type_size) {
            if (reinterpret_cast<T*>(type_data.data() + ii)->get_sequence()) {
                new (new_data.data() + prev_size) T(std::move(*reinterpret_cast<T*>(type_data.data() + ii)));
                reinterpret_cast<T*>(type_data.data() + ii)->~T();
            }
        }
        memset(type_data.data() + prev_size, 0, type_data.size() - prev_size);
        std::swap(type_data, new_data);
    }

    T* obj = new (type_data.data() + prev_size) T(std::move(args)...);
    obj->_self = handle<object>(type_index, prev_size / type_size, _index, ++_sequence);
    obj->_spawn_time = frametime();
    obj->spawn();
    return obj;
}

//------------------------------------------------------------------------------
template<typename T> object_range<T const> world::objects() const
{
    static_assert(std::is_base_of<game::object, T>::value,
        "'objects': 'T' must be derived from 'game::object'");
    return object_range<T const>(const_cast<std::vector<std::vector<byte>>&>(_objects_data)); // FIXME: const_cast
}

//------------------------------------------------------------------------------
template<typename T> object_range<T> world::objects()
{
    static_assert(std::is_base_of<game::object, T>::value,
        "'objects': 'T' must be derived from 'game::object'");
    return object_range<T>(_objects_data);
}

//------------------------------------------------------------------------------
template<typename T> T* world::get(handle<T> h) const
{
    assert(h.get_world_index() == _index);
    if (!h.get_type_index() || h.get_type_index() >= _objects_data.size()) {
        return nullptr;
    }

    assert(T::_type.is_base(h.get_type_index()));

    auto& type_data = _objects_data[h.get_type_index()];
    // Note: T is not necessarily the type of the underlying object
    std::size_t type_size = object_type::type_size(h.get_type_index());
    if (h.get_index() * type_size >= type_data.size()) {
        return nullptr;
    }

    T const* obj = reinterpret_cast<T const*>(type_data.data() + type_size * h.get_index());
    if (obj->_self.get_sequence() != h.get_sequence()) {
        return nullptr;
    }

    return const_cast<T*>(obj); // FIXME: const_cast
}

} // namespace game
