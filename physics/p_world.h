// p_world.h
//

#pragma once

#include "cm_vector.h"
#include "p_collide.h"
#include <cassert>
#include <array>
#include <functional>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace physics {

class rigid_body;
class trace;
class shape;
class world;

//------------------------------------------------------------------------------
struct collision : contact
{
    vec2 impulse; //!< Impulse that should be applied to body_b to resolve the collision

    collision() = default;
    explicit collision(contact const& c) : contact(c) {}
};

//------------------------------------------------------------------------------
class handle
{
public:
    handle()
        : _value(0)
    {}

    //! explicit conversion to bool
    explicit operator bool() const {
        return get() != nullptr;
    }

    //! retrieve a pointer to referenced rigid body from the physics world
    physics::rigid_body const* get() const;

    //! retrieve a pointer to referenced rigid body from the physics world
    physics::rigid_body* get();

    //! overloaded pointer to member operator to behave like a raw pointer
    physics::rigid_body const* operator->() const { assert(get()); return get(); }
    //! overloaded pointer to member operator to behave like a raw pointer
    physics::rigid_body* operator->() { assert(get()); return get(); }

    //! get the index of the referenced object in the world's object array
    uint64_t get_index() const { return (_value & index_mask) >> index_shift; }
    //! get a pointer to world that contains the referenced object
    physics::world* get_world() const;
    //! get the unique sequence number for the referenced object
    uint64_t get_sequence() const { return (_value & sequence_mask) >> sequence_shift; }

protected:
    friend physics::world;

    //! packed value containing object index, world index, and sequence id
    uint64_t _value;

protected:
    //! number of bits used to store the object index
    static constexpr uint64_t index_bits = 16;
    //! number of bits used to store the world index
    static constexpr uint64_t system_bits = 4;
    //! number of bits used to store the sequence id, i.e. the bits remaining after index and system
    static constexpr uint64_t sequence_bits = CHAR_BIT * sizeof(uint64_t) - index_bits - system_bits;

    //! bit offset of the object index
    static constexpr uint64_t index_shift = 0;
    //! bit offset of the world index
    static constexpr uint64_t system_shift = index_bits + index_shift;
    //! bit offset of the sequence id
    static constexpr uint64_t sequence_shift = system_bits + system_shift;

    //! bit mask of the object index
    static constexpr uint64_t index_mask = ((1ULL << index_bits) - 1) << index_shift;
    //! bit mask of the world index
    static constexpr uint64_t system_mask = ((1ULL << system_bits) - 1) << system_shift;
    //! bit mask of the sequence id
    static constexpr uint64_t sequence_mask = ((1ULL << sequence_bits) - 1) << sequence_shift;

protected:
    handle(uint64_t index_value, uint64_t system_value, uint64_t sequence_value)
        : _value((index_value << index_shift) | (system_value << system_shift) | (sequence_value << sequence_shift))
    {
        assert(index_value < (1ULL << index_bits));
        assert(system_value < (1ULL << system_bits));
        assert(sequence_value < (1ULL << sequence_bits));
    }

    uint64_t get_world_index() const { return (_value & system_mask) >> system_shift; }
};

//------------------------------------------------------------------------------
class world
{
public:
    using filter_callback_type = std::function<bool(physics::handle body_a, physics::handle body_b)>;
    using collision_callback_type = std::function<bool(physics::handle body_a, physics::handle body_b, physics::collision const& collision)>;

public:
    world(filter_callback_type filter_callback, collision_callback_type collision_callback);
    ~world();

    //void add_body(physics::rigid_body* body);
    //void remove_body(physics::rigid_body* body);

    physics::handle add_body();
    void remove_body(physics::handle const& h);

    void step(float delta_time);

protected:
    friend physics::handle;

    std::vector<physics::rigid_body> _bodies;
    std::vector<uint64_t> _bodies_sequence;
    std::vector<std::size_t> _free_bodies;

    filter_callback_type _filter_callback;
    collision_callback_type _collision_callback;

    //! World index in singletons array
    uint64_t _index;

    //! Sequence id of most recently added rigid body
    uint64_t _sequence;

    //! Maximum number of bodies that can be referenced by handle
    constexpr static int max_bodies = 1LLU << physics::handle::index_bits;
    //! Maximum number of worlds than can be referenced by handle
    constexpr static int max_worlds = 1LLU << physics::handle::system_bits;

    static std::array<physics::world*, max_worlds> _singletons;

protected:
    physics::rigid_body const* get(physics::handle const& h) const;
    physics::rigid_body* get(physics::handle const& h);

    vec2 collision_impulse(physics::rigid_body const* body_a,
                           physics::rigid_body const* body_b,
                           physics::contact const& contact) const;

    using overlap = std::pair<std::size_t, std::size_t>;

    //! Return a lexicographically sorted list of all pairs of bodies which
    //! overlap during the next `delta_time` step, including permutations.
    std::vector<overlap> generate_overlaps(float delta_time) const;
};

//------------------------------------------------------------------------------
inline physics::rigid_body const* physics::world::get(physics::handle const& h) const
{
    assert(h.get_world_index() == _index);
    assert(h.get_index() < max_bodies);
    if (h.get_index() < _bodies_sequence.size()
            && h.get_sequence() == _bodies_sequence[h.get_index()]) {
        return &_bodies[h.get_index()];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
inline physics::rigid_body* physics::world::get(physics::handle const& h)
{
    assert(h.get_world_index() == _index);
    assert(h.get_index() < max_bodies);
    if (h.get_index() < _bodies_sequence.size()
        && h.get_sequence() == _bodies_sequence[h.get_index()]) {
        return &_bodies[h.get_index()];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
inline physics::rigid_body const* physics::handle::get() const
{
    if (get_world()) {
        return get_world()->get(*this);
    } else {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
inline physics::rigid_body* physics::handle::get()
{
    if (get_world()) {
        return get_world()->get(*this);
    } else {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
inline physics::world* physics::handle::get_world() const
{
    assert(get_world_index() < physics::world::max_worlds);
    return physics::world::_singletons[get_world_index()];
}

} // namespace physics
