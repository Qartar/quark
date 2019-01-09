// g_handle.h
//

#include <cassert>
#include <climits>

////////////////////////////////////////////////////////////////////////////////
namespace game {

class object;
class world;

//------------------------------------------------------------------------------
template<typename T> class handle
{
public:
    //! default construction
    handle()
        : _value(0)
    {}

    //! construction from raw object pointer
    handle(T* object) {
        if (object) {
            _value = object->_self._value;
        } else {
            _value = 0;
        }
    }

    //! construction from handle to related type
    template<typename Y> handle(handle<Y> const& other)
        : _value(other._value)
    {
        static_assert(std::is_base_of<T, Y>::value, "cannot implicitly convert handle to unrelated type");
    }

    //! copy-assignment from raw object pointer
    handle& operator=(T* object) {
        if (object) {
            _value = object->_self._value;
        } else {
            _value = 0;
        }
        return *this;
    }

    //! copy-assignment from handle to related type
    template<typename Y> handle& operator=(handle<Y> const& other) {
        static_assert(std::is_base_of<T, Y>::value, "cannot implicitly convert handle to unrelated type");
        _value = other._value;
    }

    //! implicit conversion to bool
    operator bool() const {
        return get() != nullptr;
    }

    //! retrieve a pointer to referenced object from the game world
    T const* get() const {
        if (get_world()) {
            return get_world()->template get<T>(*this);
        } else {
            return nullptr;
        }
    }

    //! retrieve a pointer to referenced object from the game world
    T* get() {
        if (get_world()) {
            return get_world()->template get<T>(*this);
        } else {
            return nullptr;
        }
    }

    //! overloaded pointer to member operator to behave like a raw pointer
    T const* operator->() const { assert(get()); return get(); }
    //! overloaded pointer to member operator to behave like a raw pointer
    T* operator->() { assert(get()); return get(); }

    //! get the index of the referenced object in the world's object array
    uint64_t get_index() const { return (_value & index_mask) >> index_shift; }
    //! get a pointer to world that contains the referenced object
    game::world* get_world() const { return game::world::_singletons[get_world_index()]; }
    //! get the unique sequence number for the referenced object
    uint64_t get_sequence() const { return (_value & sequence_mask) >> sequence_shift; }

protected:
    friend game::world;
    template<typename> friend class handle;

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

template<typename Tx, typename Ty> bool operator==(handle<Tx> const& lhs, handle<Ty> const& rhs)
{
    return lhs.get() == rhs.get();
}

template <typename Tx, typename Ty> bool operator==(handle<Tx> const& lhs, Ty const* rhs)
{
    return lhs.get() == rhs;
}

template <typename Tx, typename Ty> bool operator==(Tx const* lhs, handle<Ty> const& rhs)
{
    return lhs == rhs.get();
}

template <typename Tx, typename Ty> bool operator!=(handle<Tx> const& lhs, Ty const* rhs)
{
    return lhs.get() != rhs;
}

template <typename Tx, typename Ty> bool operator!=(Tx const* lhs, handle<Ty> const& rhs)
{
    return lhs != rhs.get();
}

//------------------------------------------------------------------------------
/*
    A unique object handle with semantics similar to std::unique_ptr. The handle
    can be moved but not copied and automatically frees the referenced object
    when going out of scope.
*/
template<typename T> class unique_handle : public handle<T>
{
public:
    //! default construction
    unique_handle()
        : handle<T>()
    {}

    //! construction from raw object pointer
    unique_handle(T* object)
        : handle<T>(object)
    {}

    //! copy-construction from non-unique handle to related type
    template<typename Y> unique_handle(handle<Y> const& other)
        : handle<T>(other)
    {
        static_assert(std::is_base_of<T, Y>::value, "cannot implicitly convert handle to unrelated type");
    }

    //! move construction from another unique handle
    unique_handle(unique_handle<T>&& other)
        : handle<T>(other)
    {
        other._value = 0; // reset other handle to prevent double-free
    }

    //! destruction, removes object from world
    ~unique_handle() {
        if (handle<T>::get()) {
            handle<T>::get_world()->remove(handle<T>::get());
        }
    }

    //! copy-assignment from raw object pointer
    unique_handle& operator=(T* object) {
        if (handle<T>::get()) {
            handle<T>::get_world()->remove(handle<T>::get());
        }
        if (object) {
            handle<T>::_value = object->_self._value;
        } else {
            handle<T>::_value = 0;
        }
        return *this;
    }

    //! copy-assignment from non-unique handle to related type
    template<typename Y> unique_handle& operator=(handle<Y> const& object) {
        static_assert(std::is_base_of<T, Y>::value, "cannot implicitly convert handle to unrelated type");
        if (handle<T>::get()) {
            handle<T>::get_world()->remove(handle<T>::get());
        }
        handle<T>::_value = object._value;
        return *this;
    }
};

} // namespace game
