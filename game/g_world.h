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

#include "r_particle.h"

#include <array>
#include <memory>
#include <queue>
#include <type_traits>
#include <vector>

#define MAX_PLAYERS 16

constexpr const time_delta FRAMETIME = time_delta::from_seconds(0.05f);
constexpr const time_delta SECONDS_PER_DAY = time_delta::from_seconds(600.f);

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
enum class climate
{
    none,
    grassland,
    plains,
    forest,
    rocky_desert,
    sandy_desert,
    rainforest,
    boreal_forest,
    taiga,
    tundra,
};

constexpr string::literal climate_names[] = {
    "none",
    "grassland",
    "plains",
    "forest",
    "rocky desert",
    "sandy desert",
    "rainforest",
    "boreal forest",
    "taiga",
    "tunda",
};

struct climate_adjacency {
    climate a, b;
};

constexpr climate_adjacency climate_adjacencies[] = {
#if 0
    {climate::grassland, climate::plains},
    {climate::grassland, climate::forest},
    {climate::grassland, climate::rainforest},
    {climate::grassland, climate::taiga},
    {climate::plains, climate::rocky_desert},
    {climate::plains, climate::sandy_desert},
    {climate::plains, climate::forest},
    {climate::forest, climate::rainforest},
    {climate::forest, climate::boreal_forest},
    {climate::rocky_desert, climate::sandy_desert},
    {climate::boreal_forest, climate::tundra},
    {climate::boreal_forest, climate::taiga},
    {climate::taiga, climate::tundra},
#elif 0
    {climate::grassland, climate::plains},
    {climate::grassland, climate::rainforest},
    {climate::forest, climate::plains},
    {climate::forest, climate::rainforest},
#elif 0
    {climate::grassland, climate::plains},
    {climate::grassland, climate::forest},
    {climate::forest, climate::plains},
    {climate::plains, climate::rocky_desert},
    {climate::plains, climate::sandy_desert},
    {climate::rocky_desert, climate::sandy_desert},
#else
    {climate::grassland, climate::plains},
    {climate::grassland, climate::forest},
    {climate::grassland, climate::taiga},
    {climate::plains, climate::sandy_desert},
    {climate::taiga, climate::tundra},
#endif
};
constexpr std::size_t num_climate_adjacencies =
    sizeof(climate_adjacencies) / sizeof(climate_adjacencies[0]);

//------------------------------------------------------------------------------
struct hextile
{
    using index = std::size_t;
    static constexpr index invalid_index = SIZE_MAX;

    vec2i position;

    //       2 1
    //      3 6 0
    //       4 5

    std::array<int, 7> contents;
    bool is_boundary : 1,
         is_candidate : 1;

    //       2 1
    //      3 - 0
    //       4 5

    std::array<index, 6> neighbors;

    climate climate;

    //       (-1, 1 )   ( 0, 1 )
    //
    // (-1, 0 )   ( 0, 0 )   ( 1, 0 )
    //
    //       ( 0,-1 )   ( 1,-1 )

    static constexpr vec2i neighbor_offsets[6] = {
        vec2i( 1, 0), vec2i( 0, 1), vec2i(-1, 1),
        vec2i(-1, 0), vec2i( 0,-1), vec2i( 1,-1),
    };

    static constexpr vec2 vertices[6] = {
        vec2( math::sqrt3<float> / 2.f, -0.5f),
        vec2( math::sqrt3<float> / 2.f,  0.5f),
        vec2(                      0.f,  1.0f),
        vec2(-math::sqrt3<float> / 2.f,  0.5f),
        vec2(-math::sqrt3<float> / 2.f, -0.5f),
        vec2(                      0.f, -1.0f),
    };

    //! Return grid coordinates for the tile containing the given world coordinates
    static vec2i world_to_grid(vec2 v) {
        // [ sqrt(3)/3 0   ]
        // [ -1/3      2/3 ]
        float gx = v.x * (math::sqrt3<float> / 3.f) - v.y * (1.f / 3.f);
        float gy = v.y * (2.f / 3.f);
        float gz = -gx - gy;

        float rx = std::round(gx);
        float ry = std::round(gy);
        float rz = std::round(gz);

        float dx = std::abs(gx - rx);
        float dy = std::abs(gy - ry);
        float dz = std::abs(gz - rz);

        // Find the closest integer grid coordinate to the planar coordinates
        if (dx > dy && dx > dz) {
            return vec2i(int(-ry - rz), int(ry));
        } else if (dy > dz) {
            return vec2i(int(rx), int(-rx - rz));
        } else {
            return vec2i(int(rx), int(ry));
        }
    }

    //! Return world coordinates for the origin of the tile at the given grid coordinates
    static constexpr vec2 grid_to_world(vec2i v) {
        // [ sqrt(3)   0   ]
        // [ sqrt(3)/2 3/2 ]
        return vec2(v.x * math::sqrt3<float> + v.y * (math::sqrt3<float> / 2.f),
                                               v.y * (3.f / 2.f));
    }
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

    void update_usercmd(usercmd cmd, time_value time);

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

    void remove(handle<object> object);

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

    //
    // tile system
    //

    float _tile_scale;
    vec2 _tile_offset;
    std::size_t _tile_phase;

    // FIXME: This is mutable because we need the window dimensions to calculate
    // the cursor-to-world transform which is only available in the draw function.
    mutable vec2i _tile_cursor;
    int _tile_rotation;

    std::vector<hextile> _tiles;
    std::vector<hextile::index> _boundary_tiles;

    void draw_tiles(render::system* renderer) const;
    void draw_tile(render::system* renderer, hextile const& tile) const;
    hextile::index insert_tile(vec2i position, hextile const& tile);
    hextile::index insert_boundary_tile(vec2i position);

    bool match_tile(hextile::index index, hextile const& tile, int rotation) const;
    climate choose_climate(hextile::index index);

    hextile _next;
    std::vector<std::pair<hextile::index, int>> _candidates;

    usercmd _usercmd;

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
    assert(obj->is_type<T>() && "invalid type info");
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
    if (h.get_index() < _objects.size()
            && _objects[h.get_index()]
            && h.get_sequence() == _objects[h.get_index()]->_self.get_sequence()) {
        return static_cast<T*>(_objects[h.get_index()].get());
    }
    return nullptr;
}

} // namespace game
