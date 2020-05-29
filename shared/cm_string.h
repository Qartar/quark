// cm_string.h
//

#pragma once

#include <cassert>
#include <cstdlib>

////////////////////////////////////////////////////////////////////////////////
namespace string {

//------------------------------------------------------------------------------
class view
{
public:
    view() = default;
    //! Explicit conversion from null-terminated string
    explicit view(char const* c_str);
    //! Implicit conversion from char array
    template<std::size_t size> view(char (&s)[size])
        : view((char const*)s)
    {}
    //! Implicit conversion from string literal
    template<std::size_t size> view(char const (&s)[size])
        : _begin(s)
        , _end(s + size - 1)
    {}
    //! Construct from explicit range
    view(char const* begin, char const* end)
        : _begin(begin)
        , _end(end)
    {}

    //! Return a null-terminated string, making a copy if necessary
    char const* c_str() const;

    //! Returns an iterator to the beginning of the string
    char const* begin() const { return _begin; }

    //! Returns an iterator to the end of the string
    char const* end() const { return _end; }

    //! Returns the length of the string in bytes
    std::size_t length() const { return _end - _begin; }

    //! Returns the character at the given index
    char operator[](std::size_t index) const { return _begin[index]; }

    //! Returns a view of all but the first `n` characters of the string
    view skip(std::size_t n) const;

    //! Returns true if the string contains the given substring
    bool contains(string::view str) const;

    //! Returns true if the string begins with the given prefix
    bool starts_with(string::view prefix) const;

protected:
    char const* _begin;
    char const* _end;
};

//------------------------------------------------------------------------------
class literal
{
public:
    //! Implicit conversion from char array, explicitly deleted to prevent
    //! argument promotion to const array and using the constructor below.
    template<std::size_t size> literal(char (&s)[size]) = delete;
    //! Implicit conversion from string literal
    template<std::size_t size> constexpr literal(char const (&s)[size])
        : _begin(s)
        , _end(s + size - 1)
    {}

    //! Implicit conversion to string view
    operator view() const { return view{_begin, _end}; }

    //! Returns a pointer a C-style string representation
    constexpr char const* c_str() const { return _begin; }

    //! Returns an iterator to the beginning of the string
    constexpr char const* begin() const { return _begin; }

    //! Returns an iterator to the end of the string
    constexpr char const* end() const { return _end; }

    //! Returns the length of the string in bytes
    constexpr std::size_t length() const { return _end - _begin; }

    //! Returns the character at the given index
    constexpr char operator[](std::size_t index) const { return _begin[index]; }

protected:
    char const* _begin;
    char const* _end;
};

//------------------------------------------------------------------------------
class buffer
{
public:
    buffer()
        : _begin(nullptr)
        , _end(nullptr)
        , _capacity(nullptr)
    {}
    //! Explicit conversion from null-terminated string
    explicit buffer(char const* c_str);
    //! Explicit conversion from string view
    explicit buffer(view s);
    buffer(buffer&& s);
    buffer(buffer const& s);
    buffer& operator=(buffer&& s);
    buffer& operator=(buffer const& s);
    ~buffer();

    //! Implicit conversion to string view
    operator view() const { return view{_begin, _end}; }

    //! Returns a pointer a C-style string representation
    char const* c_str() const { return _begin; }

    //! Returns a mutable pointer to the internal string data
    char* data() { return _begin; }

    //! Returns an iterator to the beginning of the string
    char const* begin() const { return _begin; }

    //! Returns an iterator to the end of the string
    char const* end() const { return _end; }

    //! Returns the length of the string in bytes
    std::size_t length() const { return _end - _begin; }

    //! Returns the character at the given index
    char operator[](std::size_t index) const { return _begin[index]; }

    //! Returns the last character in the string
    char back() const { assert(_end > _begin); return *(_end - 1); }

    //! Removes the last character in the string
    void pop_back() { assert(_end > _begin); *(--_end) = '\0'; }

    //! Sets the contents of the string to the given string
    buffer& assign(char const* s, std::size_t len);

    //! Resize the internal string data for the given number of bytes, not
    //! including null terminator.
    void resize(std::size_t size);

protected:
    char* _begin;
    char* _end;
    char* _capacity;

protected:
    //! Resize the internal string data for the given number of bytes, including
    //! null terminator.
    void reserve(std::size_t size);
};

//! Returns true if the strings `lhs` and `rhs` represent the same string.
bool operator==(view lhs, view rhs);

//! Returns true if the strings `lhs` and `rhs` do not represent the same string.
bool operator!=(view lhs, view rhs);

//! Returns the length of the string `str`.
constexpr std::size_t strlen(string::literal str) { return str.length();}

//! Returns the length of the string `str`.
std::size_t strlen(string::view str);

//! Reads data from `s` and stores them according to parameter `format` into the
//! locations given by the additional arguments.
int sscanf(string::view s, string::literal fmt, ...);

//! Copies the characters of `source` to `destination`, including the terminating
//! null character (and stopping at that point). If the length of `source` is
//! greater than or equal to `destination_size` then the destination string is
//! truncated and a terminating null character is inserted at the end of the buffer.
char* strzcpy(char* destination, view source, std::size_t destination_size);

//! Copies the characters of `source` to `destination`, including the terminating
//! null character (and stopping at that point). If the length of `source` is
//! greater than or equal to `destination_size` then the destination string is
//! truncated and a terminating null character is inserted at the end of the buffer.
template<std::size_t destination_size>
char* strcpy(char (&destination)[destination_size], view source)
{
    return strzcpy(destination, source, destination_size);
}

//! Copies the characters of `source` to `destination`, including the terminating
//! null character (and stopping at that point). If the length of `source` is
//! greater than or equal to `destination_size` then the destination string is
//! truncated and a terminating null character is inserted at the end of the buffer.
template<template<class, std::size_t> typename T, std::size_t S>
char* strcpy(T<char, S>& destination, view source)
{
    return strzcpy(destination.data(), source, destination.size());
}

//! Copies the characters of `source` to `destination`, including the terminating
//! null character (and stopping at that point). If the length of `source` is
//! greater than or equal to `destination_size` then the destination string is
//! truncated and a terminating null character is inserted at the end of the buffer.
template<template<class, std::size_t> typename T, std::size_t S>
char* strcpy(T<char, S>& destination, buffer&& source)
{
    return strzcpy(destination.data(), source, destination.size());
}

//! Compares the string `str1` to the string `str2`.
int strcmp(view str1, view str2);

//! Compares the string `str1` to the string `str2` without case-sensitivity.
int stricmp(view str1, view str2);

} // namespace string

//------------------------------------------------------------------------------
string::view va(string::literal fmt, ...);

////////////////////////////////////////////////////////////////////////////////
namespace std {

template<class> struct less;

//------------------------------------------------------------------------------
template<> struct less<::string::buffer>
{
    using is_transparent = void;

    bool operator()(::string::view lhs, ::string::view rhs) const {
        return strcmp(lhs, rhs) < 0;
    }
};

//------------------------------------------------------------------------------
template<> struct less<::string::literal>
{
    using is_transparent = void;

    bool operator()(::string::view lhs, ::string::view rhs) const {
        return strcmp(lhs, rhs) < 0;
    }
};

} // namespace std
