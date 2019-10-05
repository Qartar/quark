// cm_shared.cpp
//

#include "cm_shared.h"

#include <cstdarg>
#include <cstdio> // vsnprintf

////////////////////////////////////////////////////////////////////////////////
namespace detail {

log* log::_singleton = nullptr;

//------------------------------------------------------------------------------
void log::set(log* logger)
{
    _singleton = logger;
}

//------------------------------------------------------------------------------
void log::message(char const* fmt, ...)
{
    if (_singleton) {
        constexpr int buffer_size = 8192;
        char buffer[buffer_size];

        va_list ap;

        va_start(ap, fmt);
        vsnprintf(buffer, buffer_size, fmt, ap);
        va_end(ap);

        _singleton->print(level::message, buffer);
    }
}

//------------------------------------------------------------------------------
void log::warning(char const* fmt, ...)
{
    if (_singleton) {
        constexpr int buffer_size = 8192;
        char buffer[buffer_size];

        va_list ap;

        va_start(ap, fmt);
        vsnprintf(buffer, buffer_size, fmt, ap);
        va_end(ap);

        _singleton->print(level::warning, buffer);
    }
}

//------------------------------------------------------------------------------
void log::error(char const* fmt, ...)
{
    if (_singleton) {
        constexpr int buffer_size = 8192;
        char buffer[buffer_size];

        va_list ap;

        va_start(ap, fmt);
        vsnprintf(buffer, buffer_size, fmt, ap);
        va_end(ap);

        _singleton->print(level::error, buffer);
    }
}

} // namespace detail
