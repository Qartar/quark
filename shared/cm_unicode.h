// cm_unicode.h
//

#pragma once

#include <cassert>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
namespace unicode {

//! Returns true if `s` points to an ASCII character
constexpr bool is_ascii(char const* s)
{
    return (*s & 0x80) == 0x00;
}

//! Returns true if `s` points to a valid UTF-8 encoded character
constexpr bool is_utf8(char const* s)
{
    if ((*s & 0x80) == 0x00) {
        return true;
    } else if ((*s & 0xe0) == 0xc0) {
        return ((*++s & 0xc0) == 0x80);
    } else if ((*s & 0xf0) == 0xe0) {
        return ((*++s & 0xc0) == 0x80)
            && ((*++s & 0xc0) == 0x80);
    } else if ((*s & 0xf8) == 0xf0) {
        return ((*++s & 0xc0) == 0x80)
            && ((*++s & 0xc0) == 0x80)
            && ((*++s & 0xc0) == 0x80);
    } else {
        return false;
    }
}

//! Returns the decoded codepoint for the UTF-8 character at `s`
//! and advances the pointer to the next character sequence.
constexpr char32_t decode(char const*& s)
{
    assert(is_utf8(s));
    if ((*s & 0x80) == 0x00) {
        return *s++;
    } else if ((*s & 0xe0) == 0xc0) {
        return ((*s++ & 0x1f) <<  6)
             | ((*s++ & 0x3f) <<  0);
    } else if ((*s & 0xf0) == 0xe0) {
        return ((*s++ & 0x0f) << 12)
             | ((*s++ & 0x3f) <<  6)
             | ((*s++ & 0x3f) <<  0);
    } else if ((*s & 0xf8) == 0xf0) {
        return ((*s++ & 0x07) << 18)
             | ((*s++ & 0x3f) << 12)
             | ((*s++ & 0x3f) <<  6)
             | ((*s++ & 0x3f) <<  0);
    } else {
        return *s++; // invalid utf-8
    }
}

//! Returns the number of bytes required to store the UTF-8 encoding
//! for the given codepoint and stores the encoding in the given buffer.
constexpr int encode(char32_t c, char* buf)
{
    int len = 0;
    if (c < 0x80) {
        buf[len++] = c & 0x7f;
    } else if (c < 0x800) {
        buf[len++] = ((c >>  6) & 0x1f) | 0xc0;
        buf[len++] = ((c >>  0) & 0x3f) | 0x80;
    } else if (c < 0x10000) {
        buf[len++] = ((c >> 12) & 0x0f) | 0xe0;
        buf[len++] = ((c >>  6) & 0x3f) | 0x80;
        buf[len++] = ((c >>  0) & 0x3f) | 0x80;
    } else if (c < 0x110000) {
        buf[len++] = ((c >> 18) & 0x07) | 0xf0;
        buf[len++] = ((c >> 12) & 0x3f) | 0x80;
        buf[len++] = ((c >>  6) & 0x3f) | 0x80;
        buf[len++] = ((c >>  0) & 0x3f) | 0x80;
    }
    return len;
}

//! Returns the number of bytes in the given UTF-8 character encoding
constexpr int sequence_length(char const* s)
{
    assert(is_utf8(s));
    if ((*s & 0x80) == 0x00) {
        return 1;
    } else if ((*s & 0xe0) == 0xc0) {
        return 2;
    } else if ((*s & 0xf0) == 0xe0) {
        return 3;
    } else if ((*s & 0xf8) == 0xf0) {
        return 4;
    } else {
        return 1; // invalid utf-8
    }
}

//! Returns a pointer to the next valid UTF-8 character, or end
constexpr char const* next(char const* s, char const* end)
{
    s += sequence_length(s);
    assert(s == end || is_utf8(s));
    return std::min(s, end);
}

//! Returns a pointer to the previous valid UTF-8 character, or begin
constexpr char const* prev(char const* s, char const* begin)
{
    while (begin < s && (*(--s) & 0xc0) == 0x80) {
        // no-op
    }
    assert(is_utf8(s));
    return s;
}

//! Returns true if the given UTF-8 character sequence represents a space
constexpr bool isspace(char const* s)
{
    switch (*s) {
        case ' ':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
        case '\v':
            return true;

        default:
            return false;
    }
}

//! Rewrites Unicode code points in-place for rendering
[[nodiscard]] char32_t* rewrite(char32_t* begin, char32_t* end);

} // namespace unicode
