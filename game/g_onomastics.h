// g_onomastics.h
//

#pragma once

namespace game {
namespace onomastics {

//------------------------------------------------------------------------------
enum class name_category {
    violent, // e.g. assault, onslaught, tormentor
    evil, // e.g. demon, malice, vampire
    ugly, // e.g. locust, scorpion, spider
    pretty, // e.g. rose
    heroic,
    villainous,
    resistance,
    luxurious, // e.g. diamond, emerald, jasmine, pearl
    aggressive, // e.g. ambush, conqueror, trespasser
    peaceful, // e.g. abundance, bounty, hardy, zest
    protector, // e.g. bulwark, defender, obdurate, sentinel
    creature, // e.g. auroch, demon, hawk, phoenix
    humanoid,
    sea_creature, // e.g. barracuda, kingfish, seahorse, shark
    mythical, // e.g. basilisk, cyclops, jupiter, vampire
    greek_mythology, // e.g. cyclops, icarus
    roman_mythology, // e.g. jupiter, venus
    norse_mythology,
    greek_pantheon,
    roman_pantheon,
    weapon, // e.g. broadsword, javelin, rocket
};

//------------------------------------------------------------------------------
//! Return a random name from the union of the given included categories and
//! excluding names from the union of the given excluded categories.
string::view random_name_including(
    random& r,
    name_category const* include,
    std::size_t include_size,
    name_category const* exclude,
    std::size_t exclude_size);

//------------------------------------------------------------------------------
//! Return a random name from the union of the given included categories and
//! excluding names from the union of the given excluded categories.
template<std::size_t include_size, std::size_t exclude_size>
inline string::view random_name_including(
    random& r,
    name_category (&&include)[include_size],
    name_category (&&exclude)[exclude_size])
{
    return random_name_including(r, include, include_size, exclude, exclude_size);
}

//------------------------------------------------------------------------------
//! Return a random name from the intersection of the given required categories
//! and excluding names from the union of the given excluded categories.
string::view random_name_requiring(
    random& r,
    name_category const* require,
    std::size_t require_size,
    name_category const* exclude,
    std::size_t exclude_size);

//------------------------------------------------------------------------------
//! Return a random name from the intersection of the given required categories
//! and excluding names from the union of the given excluded categories.
template<std::size_t require_size, std::size_t exclude_size>
inline string::view random_name_requiring(
    random& r,
    name_category (&&require)[require_size],
    name_category (&&exclude)[exclude_size])
{
    return random_name_requiring(r, require, require_size, exclude, exclude_size);
}

//------------------------------------------------------------------------------
void check_name_overlap();

} // namespace onomastics

using onomastics::name_category;

} // namespace game
