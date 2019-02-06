// cm_string.cpp
//

#include "cm_string.h"
#include "cm_shared.h"

#include <cstdarg>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
namespace string {

//------------------------------------------------------------------------------
view::view(char const* c_str)
    : _begin(c_str)
    , _end(c_str + ::strlen(c_str))
{}

//------------------------------------------------------------------------------
char const* view::c_str() const
{
    if (*_end == '\0') {
        return _begin;
    } else {
        // view is not null-terminated so return a null-terminated copy
        return va("%.*s", narrow_cast<int>(length()), _begin).begin();
    }
}

//------------------------------------------------------------------------------
view view::skip(std::size_t n) const
{
    return view(std::min(_begin + n, _end), _end);
}

//------------------------------------------------------------------------------
bool view::contains(string::view str) const
{
    return std::search(begin(), end(), str.begin(), str.end()) != end();
}

//------------------------------------------------------------------------------
bool view::starts_with(string::view prefix) const
{
    return length() >= prefix.length()
        && strncmp(begin(), prefix.begin(), prefix.length()) == 0;
}

//------------------------------------------------------------------------------
buffer::buffer(char const* c_str)
    : _begin(nullptr)
    , _end(nullptr)
    , _capacity(nullptr)
{
    resize(::strlen(c_str));
    strncpy(_begin, {c_str, c_str + length()}, _capacity - _begin);
}

//------------------------------------------------------------------------------
buffer::buffer(view s)
    : _begin(nullptr)
    , _end(nullptr)
    , _capacity(nullptr)
{
    resize(s.length());
    strncpy(_begin, s, _capacity - _begin);
}

//------------------------------------------------------------------------------
buffer::buffer(buffer&& s)
    : _begin(s._begin)
    , _end(s._end)
    , _capacity(s._capacity)
{
    s._begin = nullptr;
    s._end = nullptr;
    s._capacity = nullptr;
}

//------------------------------------------------------------------------------
buffer::buffer(buffer const& s)
    : _begin(nullptr)
    , _end(nullptr)
    , _capacity(nullptr)
{
    resize(s.length());
    strncpy(_begin, s, _capacity - _begin);
}

//------------------------------------------------------------------------------
buffer& buffer::operator=(buffer&& s)
{
    if (this != &s) {
        std::swap(_begin, s._begin);
        std::swap(_end, s._end);
        std::swap(_capacity, s._capacity);
        delete [] s._begin;
        s._begin = nullptr;
        s._end = nullptr;
        s._capacity = nullptr;
    }
    return *this;
}

//------------------------------------------------------------------------------
buffer& buffer::operator=(buffer const& s)
{
    if (this != &s) {
        resize(s.length());
        strncpy(_begin, s, _capacity - _begin);
    }
    return *this;
}

//------------------------------------------------------------------------------
buffer::~buffer()
{
    delete [] _begin;
}

//------------------------------------------------------------------------------
void buffer::resize(std::size_t length)
{
    reserve(length + 1);
    _end = _begin + length;
}

//------------------------------------------------------------------------------
void buffer::reserve(std::size_t size)
{
    if (_capacity < _begin + size) {
        delete [] _begin;
        _begin = new char[size];
        _capacity = _begin + size;
    }
}

//------------------------------------------------------------------------------
buffer& buffer::assign(char const* s, std::size_t len)
{
    resize(len);
    strncpy(_begin, {s, s + len}, _capacity - _begin);
    return *this;
}

//------------------------------------------------------------------------------
bool operator==(view lhs, view rhs)
{
    return strcmp(lhs, rhs) == 0;
}

//------------------------------------------------------------------------------
bool operator!=(view lhs, view rhs)
{
    return strcmp(lhs, rhs) != 0;
}

//------------------------------------------------------------------------------
std::size_t strlen(string::view str)
{
    return str.length();
}

//------------------------------------------------------------------------------
int sscanf(string::view s, string::literal fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    int len = vsscanf(s.c_str(), fmt.c_str(), ap);
    va_end(ap);

    return len;
}

//------------------------------------------------------------------------------
char* strcpy(char* destination, view source)
{
    memcpy(destination, source.begin(), source.length());
    destination[source.length()] = '\0';
    return destination;
}

//------------------------------------------------------------------------------
char* strncpy(char* destination, view source, std::size_t num)
{
    std::size_t l1 = std::min(source.length(), num);
    std::size_t l2 = num - l1;
    memcpy(destination, source.begin(), l1);
    memset(destination + l1, 0, l2);
    return destination;
}

//------------------------------------------------------------------------------
int strcmp(view str1, view str2)
{
    std::size_t l1 = str1.length();
    std::size_t l2 = str2.length();
    int d = strncmp(str1.begin(),
                    str2.begin(),
                    std::min(l1, l2));

    return d ? d :
        l1 < l2 ? -1 :
        l2 < l1 ? 1 : 0;
}

//------------------------------------------------------------------------------
int stricmp(view str1, view str2)
{
    std::size_t l1 = str1.length();
    std::size_t l2 = str2.length();
    int d = _strnicmp(str1.begin(),
                      str2.begin(),
                      std::min(l1, l2));

    return d ? d :
        l1 < l2 ? -1 :
        l2 < l1 ? 1 : 0;
}

} // namespace string

//------------------------------------------------------------------------------
string::view va (string::literal fmt, ...)
{
    constexpr int buffer_size = 16384;
    static thread_local int  index = 0;
    static thread_local char string[buffer_size];

    va_list     ap;
    char        *buf;

    int offset = index & (buffer_size-1);
    buf = string + offset;

    va_start(ap, fmt);
    int len = vsnprintf(buf, buffer_size - offset, fmt.c_str(), ap);
    va_end(ap);

    // check if a wrap is necessary
    if (len >= buffer_size - offset && len < buffer_size) {
        buf = string;
        index = 0;

        va_start(ap, fmt);
        len = vsnprintf(buf, buffer_size, fmt.c_str(), ap);
        va_end(ap);
    }

    // vsnprintf returns a negative value if an error occurs
    index += max(0, len + 1);
    return {buf, buf + len};
}
