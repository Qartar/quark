// cm_time.h
//

#pragma once

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
template<typename T> class time_base
{
public:
    time_base() = default;

    template<int64_t microseconds> struct constant {
        constexpr operator T() const {
            return T(microseconds);
        }
    };

    static constexpr constant<0> zero{};
    static constexpr constant<INT64_MIN> min{};
    static constexpr constant<INT64_MAX> max{};

    static constexpr T from_seconds(double seconds) {
        return T(static_cast<int64_t>(seconds * 1e6));
    }

    static constexpr T from_milliseconds(int64_t milliseconds) {
        return T(milliseconds * 1000);
    }

    static constexpr T from_microseconds(int64_t microseconds) {
        return T(microseconds);
    }

    constexpr double to_seconds() const {
        return _value * 1e-6;
    }

    constexpr int64_t to_milliseconds() const {
        return _value / 1000;
    }

    constexpr int64_t to_microseconds() const {
        return _value;
    }

    T& operator*=(double s) {
        _value *= s;
        return *this;
    }

    T& operator/=(double s) {
        _value /= s;
        return *this;
    }

    constexpr bool operator==(time_base<T> rhs) const {
        return _value == rhs._value;
    }

    constexpr bool operator!=(time_base<T> rhs) const {
        return _value != rhs._value;
    }

    constexpr bool operator<(time_base<T> rhs) const {
        return _value < rhs._value;
    }

    constexpr bool operator<=(time_base<T> rhs) const {
        return _value <= rhs._value;
    }

    constexpr bool operator>(time_base<T> rhs) const {
        return _value > rhs._value;
    }

    constexpr bool operator>=(time_base<T> rhs) const {
        return _value >= rhs._value;
    }

    explicit constexpr operator bool() const {
        return _value != 0;
    }

protected:
    int64_t _value;

protected:
    explicit constexpr time_base(int64_t microseconds)
        : _value(microseconds)
    {}
};

//------------------------------------------------------------------------------
template<typename Tx, typename Ty> constexpr Tx operator*(time_base<Tx> lhs, Ty rhs)
{
    return Tx::from_microseconds(static_cast<int64_t>(lhs.to_microseconds() * rhs));
}

//------------------------------------------------------------------------------
template<typename Tx, typename Ty> constexpr Ty operator*(Tx lhs, time_base<Ty> rhs)
{
    return Ty::from_microseconds(static_cast<int64_t>(lhs * rhs.to_microseconds()));
}

//------------------------------------------------------------------------------
template<typename T> constexpr T operator/(time_base<T> lhs, double rhs)
{
    return T::from_microseconds(static_cast<int64_t>(lhs.to_microseconds() / rhs));
}

//------------------------------------------------------------------------------
template<typename T> constexpr float operator/(time_base<T> lhs, time_base<T> rhs)
{
    double num = static_cast<double>(lhs.to_microseconds());
    double den = static_cast<double>(rhs.to_microseconds());
    return static_cast<float>(num / den);
}

//------------------------------------------------------------------------------
class time_delta : public time_base<time_delta>
{
public:
    time_delta() = default;

    static constexpr time_delta from_hertz(double hertz) {
        return time_delta::from_seconds(1.0 / hertz);
    }

    time_delta& operator+=(time_delta delta) {
        _value += delta.to_microseconds();
        return *this;
    };

    time_delta& operator-=(time_delta delta) {
        _value -= delta.to_microseconds();
        return *this;
    }

protected:
    friend time_base<time_delta>;

    explicit constexpr time_delta(int64_t microseconds)
        : time_base(microseconds)
    {}
};

//------------------------------------------------------------------------------
constexpr time_delta operator+(time_delta lhs, time_delta rhs)
{
    return time_delta::from_microseconds(lhs.to_microseconds() + rhs.to_microseconds());
}

//------------------------------------------------------------------------------
constexpr time_delta operator-(time_delta lhs, time_delta rhs)
{
    return time_delta::from_microseconds(lhs.to_microseconds() - rhs.to_microseconds());
}

//------------------------------------------------------------------------------
class time_value : public time_base<time_value>
{
public:
    time_value() = default;
    explicit constexpr time_value(time_delta delta)
        : time_base(delta.to_microseconds())
    {}

    time_value& operator+=(time_delta delta) {
        _value += delta.to_microseconds();
        return *this;
    };

    time_value& operator-=(time_delta delta) {
        _value -= delta.to_microseconds();
        return *this;
    }

    //! Returns the current time. This must be implemented by the application.
    static time_value current();

protected:
    friend time_base<time_value>;

    explicit constexpr time_value(int64_t microseconds)
        : time_base(microseconds)
    {}
};

//------------------------------------------------------------------------------
constexpr time_value operator+(time_value lhs, time_delta rhs)
{
    return time_value::from_microseconds(lhs.to_microseconds() + rhs.to_microseconds());
}

//------------------------------------------------------------------------------
constexpr time_value operator+(time_delta lhs, time_value rhs)
{
    return time_value::from_microseconds(lhs.to_microseconds() + rhs.to_microseconds());
}

//------------------------------------------------------------------------------
constexpr time_value operator-(time_value lhs, time_delta rhs)
{
    return time_value::from_microseconds(lhs.to_microseconds() - rhs.to_microseconds());
}

//------------------------------------------------------------------------------
constexpr time_delta operator-(time_value lhs, time_value rhs)
{
    return time_delta::from_microseconds(lhs.to_microseconds() - rhs.to_microseconds());
}

//------------------------------------------------------------------------------
constexpr float operator/(time_value lhs, time_delta rhs)
{
    double num = static_cast<double>(lhs.to_microseconds());
    double den = static_cast<double>(rhs.to_microseconds());
    return static_cast<float>(num / den);
}

//------------------------------------------------------------------------------
constexpr time_delta operator%(time_value lhs, time_delta rhs)
{
    return time_delta::from_microseconds(lhs.to_microseconds() % rhs.to_microseconds());
}
