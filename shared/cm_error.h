// cm_error.h
//

#pragma once

//------------------------------------------------------------------------------
enum class result
{
    none, //!< e.g. no-op
    success,
    failure,
};

//------------------------------------------------------------------------------
inline constexpr bool succeeded(result res)
{
    switch (res) {
        case result::none:
        case result::success:
            return true;
        default:
            return false;
    }
}

//------------------------------------------------------------------------------
inline constexpr bool failed(result res)
{
    return !succeeded(res);
}
