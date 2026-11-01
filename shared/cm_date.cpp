// cm_date.cpp
//

#include "cm_date.h"
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
namespace {

//------------------------------------------------------------------------------
constexpr bool is_leap_year(uint64_t year)
{
    return ((year % 4 == 0) && !(year % 100 == 0)) || (year % 400 == 0);
}

//------------------------------------------------------------------------------
constexpr int days_per_month(uint64_t month, uint64_t year)
{
    switch(month) {
        case 1: return 31;
        case 2: return is_leap_year(year) ? 29 : 28;
        case 3: return 31;
        case 4: return 30;
        case 5: return 31;
        case 6: return 30;
        case 7: return 31;
        case 8: return 31;
        case 9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 0;
    }
}

//------------------------------------------------------------------------------
constexpr int days_per_year(uint64_t year)
{
    return is_leap_year(year) ? 366 : 365;
}

//------------------------------------------------------------------------------
constexpr int calculate_day_of_week(uint64_t day_of_month, uint64_t month, uint64_t year)
{
    // Gauss's algorithm
    uint64_t C = year / 100;
    uint64_t Y = year % 100;
    // Day of week of Jan 1 of given year (modulo 7) starting on Monday
    uint64_t D0 = 6 + 5 * ((year - 1) % 4) + 3 * (Y - 1) + 5 * (C % 4);

    constexpr int offset[2][12] = {
        {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5},
        {0, 3, 4, 0, 2, 5, 0, 3, 6, 1, 4, 6},
    };

    uint64_t M = offset[is_leap_year(year) ? 1 : 0][month - 1];
    return (day_of_month + M + D0) % 7;
}

} // anonymous namespace

//------------------------------------------------------------------------------
date_value::date_value(int year, int month, int day_of_month, int hour, int minute, int second)
    : _microseconds(0)
    , _seconds(second)
    , _minutes(minute)
    , _hours(hour)
    , _day_of_month(day_of_month)
    , _day_of_week(calculate_day_of_week(day_of_month, month, year))
    , _month(month)
    , _year(year)
{
    assert(_seconds < 60);
    assert(_minutes < 60);
    assert(_hours < 24);
    assert(_month >= 1 && _month <= 12);
    assert(_day_of_month >= 1 && _day_of_month <= days_per_month(_month, _year));
}

//------------------------------------------------------------------------------
date_value& date_value::operator+=(time_delta time)
{
    *this = *this + time;
    return *this;
}

//------------------------------------------------------------------------------
date_value date_value::operator+(time_delta time) const
{
    uint64_t microseconds = _microseconds + time.to_microseconds();

    uint64_t seconds = _seconds + microseconds / 1000000;
    microseconds %= 1000000;

    uint64_t minutes = _minutes + seconds / 60;
    seconds %= 60;

    uint64_t hours = _hours + minutes / 60;
    minutes %= 60;

    uint64_t days = hours / 24;
    hours %= 24;

    uint64_t day_of_week = (_day_of_week + days) % 7;
    uint64_t day_of_month = _day_of_month;
    uint64_t month = _month;
    uint64_t year = _year;

    // Simplify logic below by dealing with starting on a leap day now.
    if (_month == 2 && _day_of_month == 29 && days > 0) {
        --days;
        ++month;
        day_of_month = 1;
    }

    // Advance whole years at a time
    if (month < 3) {
        while (days >= days_per_year(year)) {
            days -= days_per_year(year);
            ++year;
        }
    } else {
        while (days >= days_per_year(year + 1)) {
            days -= days_per_year(year + 1);
            ++year;
        }
    }

    // Advance whole months at a time
    day_of_month += days;
    while (day_of_month > days_per_month(month, year)) {
        day_of_month -= days_per_month(month, year);
        if (++month > 12) {
            month = 1;
            ++year;
        }
    }

    date_value result;

    result._microseconds = microseconds;
    result._seconds = seconds;
    result._minutes = minutes;
    result._hours = hours;
    result._day_of_month = day_of_month;
    result._day_of_week = day_of_week;
    result._month = month;
    result._year = year;

    return result;
}

//------------------------------------------------------------------------------
bool operator==(date_value lhs, date_value rhs) {
    return *reinterpret_cast<uint64_t const*>(&lhs) == *reinterpret_cast<uint64_t const*>(&rhs);
}

//------------------------------------------------------------------------------
bool operator!=(date_value lhs, date_value rhs) {
    return *reinterpret_cast<uint64_t const*>(&lhs) != *reinterpret_cast<uint64_t const*>(&rhs);
}

//------------------------------------------------------------------------------
bool operator<(date_value lhs, date_value rhs) {
    return *reinterpret_cast<uint64_t const*>(&lhs) < *reinterpret_cast<uint64_t const*>(&rhs);
}

//------------------------------------------------------------------------------
bool operator<=(date_value lhs, date_value rhs) {
    return *reinterpret_cast<uint64_t const*>(&lhs) <= *reinterpret_cast<uint64_t const*>(&rhs);
}

//------------------------------------------------------------------------------
bool operator>(date_value lhs, date_value rhs) {
    return *reinterpret_cast<uint64_t const*>(&lhs) > *reinterpret_cast<uint64_t const*>(&rhs);
}

//------------------------------------------------------------------------------
bool operator>=(date_value lhs, date_value rhs) {
    return *reinterpret_cast<uint64_t const*>(&lhs) >= *reinterpret_cast<uint64_t const*>(&rhs);
}
