// cm_enumflag.h
//

#pragma once

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////
namespace detail {

//------------------------------------------------------------------------------
template<std::size_t size> struct enum_value;
template<> struct enum_value<1> { using type = std::uint8_t; };
template<> struct enum_value<2> { using type = std::uint16_t; };
template<> struct enum_value<4> { using type = std::uint32_t; };
template<> struct enum_value<8> { using type = std::uint64_t; };

//------------------------------------------------------------------------------
template<typename T> using enum_value_type = typename enum_value<sizeof(T)>::type;

} // namespace detail

//----------------------------------------------------------------------------------------------------------
#define ENUM_FLAG_OPERATORS_IMPL(ENUM_TYPE, VALUE_TYPE, ...)                                                \
inline __VA_ARGS__ constexpr ENUM_TYPE operator|(ENUM_TYPE lhs, ENUM_TYPE rhs) {                            \
    return static_cast<ENUM_TYPE>(static_cast<VALUE_TYPE>(lhs) | static_cast<VALUE_TYPE>(rhs));             \
}                                                                                                           \
inline __VA_ARGS__ constexpr ENUM_TYPE operator&(ENUM_TYPE lhs, ENUM_TYPE rhs) {                            \
    return static_cast<ENUM_TYPE>(static_cast<VALUE_TYPE>(lhs) & static_cast<VALUE_TYPE>(rhs));             \
}                                                                                                           \
inline __VA_ARGS__ constexpr ENUM_TYPE operator^(ENUM_TYPE lhs, ENUM_TYPE rhs) {                            \
    return static_cast<ENUM_TYPE>(static_cast<VALUE_TYPE>(lhs) ^ static_cast<VALUE_TYPE>(rhs));             \
}                                                                                                           \
inline __VA_ARGS__ constexpr ENUM_TYPE operator~(ENUM_TYPE lhs) {                                           \
    return static_cast<ENUM_TYPE>(~static_cast<VALUE_TYPE>(lhs));                                           \
}                                                                                                           \
inline __VA_ARGS__ ENUM_TYPE& operator|=(ENUM_TYPE& lhs, ENUM_TYPE rhs) { lhs = lhs | rhs; return lhs; }    \
inline __VA_ARGS__ ENUM_TYPE& operator&=(ENUM_TYPE& lhs, ENUM_TYPE rhs) { lhs = lhs & rhs; return lhs; }    \
inline __VA_ARGS__ ENUM_TYPE& operator^=(ENUM_TYPE& lhs, ENUM_TYPE rhs) { lhs = lhs ^ rhs; return lhs; }    \
inline __VA_ARGS__ bool operator!(ENUM_TYPE lhs) { return lhs == ENUM_TYPE{}; }

//----------------------------------------------------------------------------------------------------------
#define ENUM_FLAG_OPERATORS(ENUM)                                                                           \
    ENUM_FLAG_OPERATORS_IMPL(ENUM, typename ::detail::enum_value_type<ENUM>)

//----------------------------------------------------------------------------------------------------------
#define ENUM_FLAG_FRIEND_OPERATORS(ENUM)                                                                    \
    ENUM_FLAG_OPERATORS_IMPL(ENUM, typename ::detail::enum_value_type<ENUM>, friend)
