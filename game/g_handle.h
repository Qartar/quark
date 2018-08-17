// g_handle.h
//

////////////////////////////////////////////////////////////////////////////////
namespace game {

class object;
class world;

//------------------------------------------------------------------------------
template<typename T> class handle
{
public:
    handle()
        : _value(0)
    {}

    handle(T* object) {
        if (object) {
            _value = object->_self._value;
        } else {
            _value = 0;
        }
    }

    handle& operator=(T* object) {
        if (object) {
            _value = object->_self._value;
        } else {
            _value = 0;
        }
        return *this;
    }

    operator bool() const {
        if (get_world()) {
            return !!get_world()->get<T>(*this);
        } else {
            return false;
        }
    }

    T const* get() const {
        if (get_world()) {
            return get_world()->get<T>(*this);
        } else {
            return nullptr;
        }
    }

    T* get() {
        if (get_world()) {
            return get_world()->get<T>(*this);
        } else {
            return nullptr;
        }
    }

    T const* operator->() const { return get(); }
    T* operator->() { return get(); }

    uint64_t get_index() const { return (_value & index_mask) >> index_shift; }
    game::world* get_world() const { return game::world::_singletons[get_world_index()]; }
    uint64_t get_sequence() const { return (_value & sequence_mask) >> sequence_shift; }

protected:
    friend game::world;
    template<typename> friend class handle;

    uint64_t _value;

protected:
    static constexpr uint64_t index_bits = 16;
    static constexpr uint64_t system_bits = 4;
    static constexpr uint64_t sequence_bits = CHAR_BIT * sizeof(uint64_t) - index_bits - system_bits;

    static constexpr uint64_t index_shift = 0;
    static constexpr uint64_t system_shift = index_bits + index_shift;
    static constexpr uint64_t sequence_shift = system_bits + system_shift;

    static constexpr uint64_t index_mask = ((1ULL << index_bits) - 1) << index_shift;
    static constexpr uint64_t system_mask = ((1ULL << system_bits) - 1) << system_shift;
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

} // namespace game
