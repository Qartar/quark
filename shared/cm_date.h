// cm_date.h
//

#pragma once

#include "cm_time.h"
#include <cstdint>

////////////////////////////////////////////////////////////////////////////////
//! Date representation using Gregorian calendar. Ignores leap seconds. Has the
//! same resolution as time_value and time_delta (ie. microseconds).
class date_value
{
public:
    date_value() = default;
    date_value(int year, int month = 1, int day_of_month = 1, int hour = 0, int minute = 0, int second = 0);

    date_value& operator+=(time_delta time);
    date_value operator+(time_delta time) const;

    friend bool operator==(date_value lhs, date_value rhs);
    friend bool operator!=(date_value lhs, date_value rhs);
    friend bool operator<(date_value lhs, date_value rhs);
    friend bool operator<=(date_value lhs, date_value rhs);
    friend bool operator>(date_value lhs, date_value rhs);
    friend bool operator>=(date_value lhs, date_value rhs);

    int microseconds() const { return _microseconds; }
    int seconds() const { return _seconds; }
    int minutes() const { return _minutes; }
    int hours() const { return _hours; }
    int day_of_month() const { return _day_of_month; }
    int day_of_week() const { return _day_of_week; }
    int month() const { return _month; }
    int year() const { return _year; }

protected:
    uint64_t _microseconds : 20, //!< 0-999999
        _seconds : 6, //!< 0-59
        _minutes : 6, //!< 0-59
        _hours : 5, //!< 0-23
        _day_of_month : 5, //!< 1-31
        _day_of_week : 3, //!< 0-6, starting on Monday
        _month : 4, //!< 1-12
        _year : 15; //!< 0-32767
};
