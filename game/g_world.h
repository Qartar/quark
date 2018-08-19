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
        return _begin != other._begin;
    }

    type* operator*() const {
        assert(_begin < _end);
        return _begin->get();
    }

    object_iterator& operator++() {
        return next();
    }

    //  required for forward_iterator

    object_iterator operator++(int) const {
        return object_iterator(*this).next();
    }

protected:
    //! game::world uses a vector of unique_ptrs so convert the template type
    //! to the appropriate unique_ptr type depending on constness.
    using pointer_type = typename std::conditional<
                             std::is_const<type>::value,
                             std::unique_ptr<typename std::remove_const<type>::type> const*,
                             std::unique_ptr<type>*>::type;

    pointer_type _begin;
    pointer_type _end;

protected:
    template<typename> friend class object_range;

    object_iterator(pointer_type begin, pointer_type end)
        : _begin(begin)
        , _end(end)
    {
        // advance to the first active object
        if (_begin < _end) {
            --_begin;
            next();
        }
    }

    object_iterator& next() {
        assert(_begin < _end);
        do {
            ++_begin;
        } while (_begin < _end && _begin->get() == nullptr);
        return *this;
    }
};

//------------------------------------------------------------------------------
template<typename type> class object_range
{
public:
    using iterator_type = object_iterator<type>;
    using pointer_type = typename iterator_type::pointer_type;

    iterator_type begin() const { return iterator_type(_begin, _end); }
    iterator_type end() const { return iterator_type(_end, _end); }

protected:
    friend world;

    pointer_type _begin;
    pointer_type _end;

protected:
    object_range(pointer_type begin, pointer_type end)
        : _begin(begin)
        , _end(end)
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
    object_range<object const> objects() const;

    //! Return an iterator over all active objects in the world
    object_range<object> objects();

    //! Return a handle to the object with the given sequence id
    template<typename T> handle<T> find(uint64_t sequence) const;

    random& get_random() { return _random; }

    void remove(object* object);

    void add_sound(sound::asset sound_asset, vec2 position, float volume = 1.0f);
    void add_effect(time_value time, effect_type type, vec2 position, vec2 direction = vec2(0,0), float strength = 1);
    void add_trail_effect(effect_type type, vec2 position, vec2 old_position, vec2 direction = vec2(0,0), float strength = 1);

    void add_body(game::object* owner, physics::rigid_body* body);
    void remove_body(physics::rigid_body* body);

    game::object* trace(physics::contact& contact, vec2 start, vec2 end, game::object const* ignore = nullptr) const;

    int framenum() const { return _framenum; }
    time_value frametime() const { return time_value(_framenum * FRAMETIME); }

private:
    //! Sparse array of objects in the world, resized as needed
    std::vector<std::unique_ptr<object>> _objects;

    //! Objects pending removal
    std::queue<object*> _removed;

    //! World index in singletons array
    uint64_t _index;

    //! Sequence id of most recently spawned object
    uint64_t _sequence;

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

    //! Random number generator
    random _random;

    bool physics_filter_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b);
    bool physics_collide_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b, physics::collision const& collision);

    //
    // particle system
    //

    mutable std::vector<render::particle> _particles;

    render::particle* add_particle(time_value time);
    void free_particle (render::particle* particle) const;

    void draw_particles(render::system* renderer, time_value time) const;

    int _framenum;

    network::message_storage _message;

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

    uint64_t obj_index = 0;
    // try to find an unused slot in the objects array
    for (; obj_index < _objects.size(); ++obj_index) {
        if (!_objects[obj_index]) {
            break;
        }
    }

    if (obj_index == _objects.size()) {
        assert(_objects.size() < max_objects);
        _objects.push_back(std::make_unique<T>(std::move(args)...));
    } else {
        _objects[obj_index] = std::make_unique<T>(std::move(args)...);
    }

    T* obj = static_cast<T*>(_objects[obj_index].get());
    obj->_self = handle<object>(obj_index, _index, ++_sequence);
    obj->_spawn_time = frametime();
    obj->spawn();
    return obj;
}

//------------------------------------------------------------------------------
template<typename T> handle<T> world::find(uint64_t sequence) const
{
    if (!sequence) {
        return handle<T>(0, _index, 0);
    }

    for (auto& obj : _objects) {
        if (obj.get() && sequence == obj->_self.get_sequence()) {
            return obj->_self;
        }
    }

    return handle<T>(0, _index, 0);
}

//------------------------------------------------------------------------------
template<typename T> T* world::get(handle<T> h) const
{
    assert(h.get_world_index() == _index);
    assert(h.get_index() < max_objects);
    if (_objects[h.get_index()] && h.get_sequence() == _objects[h.get_index()]->_self.get_sequence()) {
        return static_cast<T*>(_objects[h.get_index()].get());
    }
    return nullptr;
}

} // namespace game
