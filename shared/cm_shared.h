// cm_shared.h
//

#pragma once

#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

#define APP_CLASSNAME   L"Quark"

#define PORT_SERVER     28101
#define PORT_CLIENT     28110

//------------------------------------------------------------------------------
using byte = std::uint8_t;
using word = std::uint16_t;

#include "cm_vector.h"
#include "cm_matrix.h"
#include "cm_color.h"
#include "cm_error.h"

#define MAX_STRING      1024
#define LONG_STRING     256
#define SHORT_STRING    32

//------------------------------------------------------------------------------
namespace math {

template<typename T> constexpr T ln2 = T(0.693147180559945309417);

template<typename T> constexpr T pi = T(3.14159265358979323846);

template<typename T> constexpr T sqrt2 = T(1.41421356237309504880);

template<typename T> constexpr T deg2rad(T value) { return value * pi<T> / T(180.0); }

template<typename T> constexpr T rad2deg(T value) { return value * T(180.0) / pi<T>; }

} // namespace math

template<typename T> constexpr T square(T value) { return value * value; }

template<typename T, typename Y> constexpr T clamp(Y value, T min, T max) { return (value < min) ? min : (value > max) ? max : T(value); }

template<typename T> constexpr T min(T a, T b) { return a < b ? a : b; }
template<typename T> constexpr T max(T a, T b) { return a > b ? a : b; }

//------------------------------------------------------------------------------
template<typename T, std::size_t size> constexpr std::size_t countof(T const (&)[size])
{
    return size;
}

//------------------------------------------------------------------------------
template<typename T> constexpr std::size_t countof()
{
    return countof(T{});
}

//------------------------------------------------------------------------------
template<typename T, typename Y> constexpr T narrow_cast(Y value)
{
    assert(static_cast<Y>(static_cast<T>(value)) == value);
    return static_cast<T>(value);
}

//------------------------------------------------------------------------------
namespace detail {

class log
{
public:
    enum class level {
        message,
        warning,
        error,
    };

    //! set the logging singleton
    static void set(log* logger);

    static void message(char const* fmt, ...);
    static void warning(char const* fmt, ...);
    static void error(char const* fmt, ...);

protected:
    virtual void print(level level, char const* message) = 0;

private:
    static log* _singleton;
};

} // namespace detail

using namespace detail; // workaround for namespace collision with <cmath>
