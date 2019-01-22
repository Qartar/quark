// cm_unicode.cpp
//

#include "cm_unicode.h"

#include <map>

////////////////////////////////////////////////////////////////////////////////
namespace unicode {

namespace {

struct join {
    enum join_type {
        none    = 0,
        final   = 1, //!< joins to previous character
        initial = 2, //!< joins to next character
        medial  = 3, //!< joins in either direction
    };
    int isolated_form;
    int final_form;
    int initial_form;
    int medial_form;
    join_type type;
    int operator[](int index) const { return (&isolated_form)[index]; }
};

const std::map<int, join> joining({
    { 0x0627, {0xfe8d, 0xfe8e,      0,      0, join::final} }, // arabic letter alef
    { 0x0628, {0xfe8f, 0xfe90, 0xfe91, 0xfe92, join::medial} }, // arabic letter beh
    { 0x0629, {0xfe93, 0xfe94,      0,      0, join::final} }, // arabic letter teh martuba
    { 0x062a, {0xfe95, 0xfe96, 0xfe97, 0xfe98, join::medial} }, // arabic letter teh
    { 0x062b, {0xfe99, 0xfe9a, 0xfe9b, 0xfe9c, join::medial} }, // arabic letter theh
    { 0x062c, {0xfe9d, 0xfe9e, 0xfe9f, 0xfea0, join::medial} }, // arabic letter jeem
    { 0x062d, {0xfea1, 0xfea2, 0xfea3, 0xfea4, join::medial} }, // arabic letter hah
    { 0x062e, {0xfea5, 0xfea6, 0xfea7, 0xfea8, join::medial} }, // arabic letter khah
    { 0x062f, {0xfea9, 0xfeaa,      0,      0, join::final} }, // arabic letter dal
    { 0x0630, {0xfeab, 0xfeac,      0,      0, join::final} }, // arabic letter thal
    { 0x0631, {0xfead, 0xfeae,      0,      0, join::final} }, // arabic letter reh
    { 0x0632, {0xfeaf, 0xfeb0,      0,      0, join::final} }, // arabic letter zain
    { 0x0633, {0xfeb1, 0xfeb2, 0xfeb3, 0xfeb4, join::medial} }, // arabic letter seen
    { 0x0634, {0xfeb5, 0xfeb6, 0xfeb7, 0xfeb8, join::medial} }, // arabic letter sheen
    { 0x0635, {0xfeb9, 0xfeba, 0xfebb, 0xfebc, join::medial} }, // arabic letter sad
    { 0x0636, {0xfebd, 0xfebe, 0xfebf, 0xfec0, join::medial} }, // arabic letter dad
    { 0x0637, {0xfec1, 0xfec2, 0xfec3, 0xfec4, join::medial} }, // arabic letter tah
    { 0x0638, {0xfec5, 0xfec6, 0xfec7, 0xfec8, join::medial} }, // arabic letter zah
    { 0x0639, {0xfec9, 0xfeca, 0xfecb, 0xfecc, join::medial} }, // arabic letter ain
    { 0x063a, {0xfecd, 0xfece, 0xfecf, 0xfed0, join::medial} }, // arabic letter ghain
    { 0x0641, {0xfed1, 0xfed2, 0xfed3, 0xfed4, join::medial} }, // arabic letter feh
    { 0x0642, {0xfed5, 0xfed6, 0xfed7, 0xfed8, join::medial} }, // arabic letter qaf
    { 0x0643, {0xfed9, 0xfeda, 0xfedb, 0xfedc, join::medial} }, // arabic letter kaf
    { 0x0644, {0xfedd, 0xfede, 0xfedf, 0xfee0, join::medial} }, // arabic letter lam
    { 0x0645, {0xfee1, 0xfee2, 0xfee3, 0xfee4, join::medial} }, // arabic letter meem
    { 0x0646, {0xfee5, 0xfee6, 0xfee7, 0xfee8, join::medial} }, // arabic letter noon
    { 0x0647, {0xfee9, 0xfeea, 0xfeeb, 0xfeec, join::medial} }, // arabic letter heh
    { 0x0648, {0xfeed, 0xfeee,      0,      0, join::final} }, // arabic letter waw
    { 0x0649, {0xfeef, 0xfef0,      0,      0, join::final} }, // arabic letter alef maksura
    { 0x064a, {0xfef1, 0xfef2, 0xfef3, 0xfef4, join::medial} }, // arabic letter yeh
});

join join_info(int cp)
{
    auto it = joining.find(cp);
    return it == joining.cend() ? join{} : it->second;
}

constexpr bool is_right_to_left(int cp)
{
    if ((cp >= 0x0590 && cp <= 0x05ff)
        || (cp >= 0x0600 && cp <= 0x06ff)
        || (cp >= 0x0700 && cp <= 0x074f)
        || (cp >= 0x0750 && cp <= 0x077f)
        || (cp >= 0xfe70 && cp <= 0xfeff)) {
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

void rejoin(int* begin, int* end)
{
    join prev{};
    join current = (begin < end) ? join_info(*begin) : join{};
    while (begin < end) {
        join next = (begin + 1 < end) ? join_info(*(begin + 1)) : join{};
        int type = join::none;

        if ((prev.type & join::initial) && (current.type & join::final)) {
            type |= join::final;
        }

        if ((current.type & join::initial) && (next.type & join::final)) {
            type |= join::initial;
        }

        if (type) {
            *begin = current[type];
        }
        prev = current;
        current = next;
        ++begin;
    }
}

} // anonymous namespace

//------------------------------------------------------------------------------
void rewrite(int* begin, int* end)
{
    rejoin(begin, end);

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
