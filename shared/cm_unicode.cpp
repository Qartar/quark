// cm_unicode.cpp
//

#include "cm_unicode.h"

////////////////////////////////////////////////////////////////////////////////
namespace unicode {

namespace {

constexpr bool is_right_to_left(int cp)
{
    if ((cp >= 0x0590 && cp <= 0x05ff)
        || (cp >= 0x0600 && cp <= 0x06ff)
        || (cp >= 0x0700 && cp <= 0x074f)
        || (cp >= 0x0750 && cp <= 0x077f)) {
        return true;
    } else {
        return false;
    }
}

constexpr bool is_neutral(int cp)
{
    if (cp == ' ') {
        return true;
    } else {
        return false;
    }
}

constexpr bool is_left_to_right(int cp)
{
    return !is_right_to_left(cp) && !is_neutral(cp);
}

} // anonymous namespace

//------------------------------------------------------------------------------
void rewrite(int* begin, int* end)
{
    while (begin < end) {
        // left-to-right
        int* p1 = begin;
        while (p1 < end && !is_right_to_left(*p1)) {
            ++p1;
        }

        // right-to-left
        int* p2 = p1;
        while (p2 < end && !is_left_to_right(*p2)) {
            ++p2;
        }

        // rewind past neutral characters
        while (p1 < p2 && is_neutral(*(p2 - 1))) {
            --p2;
        }

        begin = p2;
        // reverse right-to-left section
        while (p1 < p2) {
            std::swap(*p1++, *--p2);
        }
    }
}

} // namespace unicode
