// g_traits.h
//

#pragma once

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
enum class trait {
    edible,
    burnable,
    foragable,
    harvestable,
    collectable,
};

//------------------------------------------------------------------------------
struct traits
{
public:
    template<typename... T>
    traits(T... args)
        : _bitfield(trait_bits(args...))
    {}

    bool has(trait t) const {
        return !!(_bitfield & trait_bits(t));
    }

    void set(trait t) {
        _bitfield |= trait_bits(t);
    }

private:
    using bitfield_type = uint64_t;
    bitfield_type _bitfield;

private:
    template<typename... T> 
    static bitfield_type trait_bits(trait t, T... args)
    {
        return trait_bits(t) | trait_bits(args...);
    }
    static bitfield_type trait_bits(trait t)
    {
        return bitfield_type(1) << static_cast<bitfield_type>(t);
    }
    static bitfield_type trait_bits()
    {
        return 0;
    }
};

} // namespace game
