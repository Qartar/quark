// cm_unicode.cpp
//

#include "cm_unicode.h"

#include <map>

////////////////////////////////////////////////////////////////////////////////
namespace unicode {

namespace {

struct join {
    enum form {
        isolated = 0b00,
        final    = 0b01, //!< joins to previous character
        initial  = 0b10, //!< joins to next character
        medial   = 0b11, //!< joins in both directions
    };
    // Joining Type as defined in the Unicode Standard, Chapter 9.2
    enum class type {
        non_joining, //!<
        right_joining, //!< joins character to the right in visual order
        left_joining, //!< joins character to the left in visual order
        dual_joining, //!< joins in either direction
        join_causing, //!< joins in either direction but does not change form
        transparent, //!< ignored for joining
    };
    char32_t isolated_form;
    char32_t final_form;
    char32_t initial_form;
    char32_t medial_form;
    type joining_type;
    char32_t operator[](int index) const { return (&isolated_form)[index]; }
};

const std::map<int, join> joining({
    // Arabic Block
    { 0x0627, {0xfe8d, 0xfe8e,      0,      0, join::type::right_joining} }, // arabic letter alef
    { 0x0628, {0xfe8f, 0xfe90, 0xfe91, 0xfe92, join::type::dual_joining} }, // arabic letter beh
    { 0x0629, {0xfe93, 0xfe94,      0,      0, join::type::right_joining} }, // arabic letter teh martuba
    { 0x062a, {0xfe95, 0xfe96, 0xfe97, 0xfe98, join::type::dual_joining} }, // arabic letter teh
    { 0x062b, {0xfe99, 0xfe9a, 0xfe9b, 0xfe9c, join::type::dual_joining} }, // arabic letter theh
    { 0x062c, {0xfe9d, 0xfe9e, 0xfe9f, 0xfea0, join::type::dual_joining} }, // arabic letter jeem
    { 0x062d, {0xfea1, 0xfea2, 0xfea3, 0xfea4, join::type::dual_joining} }, // arabic letter hah
    { 0x062e, {0xfea5, 0xfea6, 0xfea7, 0xfea8, join::type::dual_joining} }, // arabic letter khah
    { 0x062f, {0xfea9, 0xfeaa,      0,      0, join::type::right_joining} }, // arabic letter dal
    { 0x0630, {0xfeab, 0xfeac,      0,      0, join::type::right_joining} }, // arabic letter thal
    { 0x0631, {0xfead, 0xfeae,      0,      0, join::type::right_joining} }, // arabic letter reh
    { 0x0632, {0xfeaf, 0xfeb0,      0,      0, join::type::right_joining} }, // arabic letter zain
    { 0x0633, {0xfeb1, 0xfeb2, 0xfeb3, 0xfeb4, join::type::dual_joining} }, // arabic letter seen
    { 0x0634, {0xfeb5, 0xfeb6, 0xfeb7, 0xfeb8, join::type::dual_joining} }, // arabic letter sheen
    { 0x0635, {0xfeb9, 0xfeba, 0xfebb, 0xfebc, join::type::dual_joining} }, // arabic letter sad
    { 0x0636, {0xfebd, 0xfebe, 0xfebf, 0xfec0, join::type::dual_joining} }, // arabic letter dad
    { 0x0637, {0xfec1, 0xfec2, 0xfec3, 0xfec4, join::type::dual_joining} }, // arabic letter tah
    { 0x0638, {0xfec5, 0xfec6, 0xfec7, 0xfec8, join::type::dual_joining} }, // arabic letter zah
    { 0x0639, {0xfec9, 0xfeca, 0xfecb, 0xfecc, join::type::dual_joining} }, // arabic letter ain
    { 0x063a, {0xfecd, 0xfece, 0xfecf, 0xfed0, join::type::dual_joining} }, // arabic letter ghain
    { 0x0641, {0xfed1, 0xfed2, 0xfed3, 0xfed4, join::type::dual_joining} }, // arabic letter feh
    { 0x0642, {0xfed5, 0xfed6, 0xfed7, 0xfed8, join::type::dual_joining} }, // arabic letter qaf
    { 0x0643, {0xfed9, 0xfeda, 0xfedb, 0xfedc, join::type::dual_joining} }, // arabic letter kaf
    { 0x0644, {0xfedd, 0xfede, 0xfedf, 0xfee0, join::type::dual_joining} }, // arabic letter lam
    { 0x0645, {0xfee1, 0xfee2, 0xfee3, 0xfee4, join::type::dual_joining} }, // arabic letter meem
    { 0x0646, {0xfee5, 0xfee6, 0xfee7, 0xfee8, join::type::dual_joining} }, // arabic letter noon
    { 0x0647, {0xfee9, 0xfeea, 0xfeeb, 0xfeec, join::type::dual_joining} }, // arabic letter heh
    { 0x0648, {0xfeed, 0xfeee,      0,      0, join::type::right_joining} }, // arabic letter waw
    { 0x0649, {0xfeef, 0xfef0,      0,      0, join::type::right_joining} }, // arabic letter alef maksura
    { 0x064a, {0xfef1, 0xfef2, 0xfef3, 0xfef4, join::type::dual_joining} }, // arabic letter yeh
    // Arabic Presentation Forms-B Block (via ligature replacement)
    { 0xfefb, {0xfefb, 0xfefc,      0,      0, join::type::right_joining} }, // arabic ligature lam with alef
});

const std::map<std::pair<char32_t, char32_t>, char32_t> ligatures({
    { {0x0644, 0x0627}, 0xfefb }, // arabic ligature lam with alef
});

join join_info(char32_t cp)
{
    auto it = joining.find(cp);
    return it == joining.cend() ? join{cp} : it->second;
}

constexpr bool is_hebrew_block(char32_t cp) { return cp >= 0x0590 && cp <= 0x05ff; }
constexpr bool is_arabic_block(char32_t cp) { return cp >= 0x0600 && cp <= 0x06ff; }
constexpr bool is_syriac_block(char32_t cp) { return cp >= 0x0700 && cp <= 0x074f; }
constexpr bool is_arabic_supplement_block(char32_t cp) { return cp >= 0x0750 && cp <= 0x077f; }
constexpr bool is_arabic_extended_a_block(char32_t cp) { return cp >= 0x08a0 && cp <= 0x08ff; }
constexpr bool is_arabic_presentation_forms_a_block(char32_t cp) { return cp >= 0xfb50 && cp <= 0xfdff; }
constexpr bool is_arabic_presentation_forms_b_block(char32_t cp) { return cp >= 0xfe70 && cp <= 0xfeff; }

constexpr bool is_right_to_left(char32_t cp)
{
    if (is_hebrew_block(cp)
        || is_arabic_block(cp)
        || is_syriac_block(cp)
        || is_arabic_supplement_block(cp)
        || is_arabic_extended_a_block(cp)
        || is_arabic_presentation_forms_a_block(cp)
        || is_arabic_presentation_forms_b_block(cp)) {
        return true;
    } else {
        return false;
    }
}

constexpr bool is_neutral(char32_t cp)
{
    if (cp == ' ') {
        return true;
    } else {
        return false;
    }
}

constexpr bool is_left_to_right(char32_t cp)
{
    return !is_right_to_left(cp) && !is_neutral(cp);
}

//! Determines whether two adjacent characters can be joined.
constexpr bool can_join(join const& lhs, join const& rhs)
{
    switch (lhs.joining_type) {
        case join::type::right_joining:
        case join::type::dual_joining:
        case join::type::join_causing:
            switch (rhs.joining_type) {
                case join::type::left_joining:
                case join::type::dual_joining:
                case join::type::join_causing:
                    return true;
                default:
                    // handling of transparent characters must be performed by caller
                    assert(rhs.joining_type != join::type::transparent);
                    return false;
            }
        default:
            // handling of transparent characters must be performed by caller
            assert(lhs.joining_type != join::type::transparent);
            return false;
    }
}

char32_t* reshape(char32_t* begin, char32_t* end)
{
    join prev{};
    join current = (begin < end) ? join_info(*begin) : join{};
    char32_t* out = begin;
    while (begin < end) {
        // replace ligatures with current and next characters
        if (begin + 1 < end) {
            auto it = ligatures.find({*begin, *(begin + 1)});
            if (it != ligatures.cend()) {
                current = join_info(it->second);
                ++begin; // skip second character in ligature
            }
        }

        join next = (begin + 1 < end) ? join_info(*(begin + 1)) : join{};
        int type = join::isolated;

        // Note: assumes right-to-left directionality
        if (can_join(current, prev)) {
            type |= join::final;
        }

        // Note: assumes right-to-left directionality
        if (can_join(next, current)) {
            type |= join::initial;
        }

        *out++ = current[type];
        prev = current;
        current = next;
        ++begin;
    }

    return out;
}

char32_t* reorder(char32_t* begin, char32_t* end)
{
    while (begin < end) {
        // left-to-right
        char32_t* p1 = begin;
        while (p1 < end && !is_right_to_left(*p1)) {
            ++p1;
        }

        // right-to-left
        char32_t* p2 = p1;
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

    return end;
}

} // anonymous namespace

//------------------------------------------------------------------------------
char32_t* rewrite(char32_t* begin, char32_t* end)
{
    //! replace contextual characters with presentation forms
    end = reshape(begin, end);

    //! reorder characters for bidirectional text
    return reorder(begin, end);
}

} // namespace unicode
