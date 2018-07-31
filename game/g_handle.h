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
        : _world(nullptr)
        , _spawn_id(0)
    {}

    handle(T* object) {
        if (object) {
            _world = object->_world;
            _spawn_id = object->_spawn_id;
        } else {
            _world = nullptr;
            _spawn_id = 0;
        }
    }

    handle& operator=(T* object) {
        if (object) {
            _world = object->_world;
            _spawn_id = object->_spawn_id;
        } else {
            _world = nullptr;
            _spawn_id = 0;
        }
        return *this;
    }

    operator bool() const {
        return _world && _world->find_object(_spawn_id);
    }

    T const* get() const {
        if (_world) {
            return static_cast<T*>(_world->find_object(_spawn_id));
        } else {
            return nullptr;
        }
    }
    T* get() {
        if (_world) {
            return static_cast<T*>(_world->find_object(_spawn_id));
        } else {
            return nullptr;
        }
    }

    T const* operator->() const { return get(); }
    T* operator->() { return get(); }

protected:
    game::world* _world;
    std::size_t _spawn_id;
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
