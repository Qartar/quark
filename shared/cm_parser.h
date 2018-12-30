// cm_parser.h
//

#pragma once

#include "cm_string.h"
#include <cassert>
#include <variant>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace parser {

//------------------------------------------------------------------------------
class text
{
public:
    text() = default;
    explicit text(string::view string);
    text(char const* begin, char const* end)
        : text(string::view{begin, end})
    {}
    text(text&& other);

    text& operator=(text&& other);

    std::vector<string::view> const& tokens() const { return _tokens; }

protected:
    std::vector<char> _buffer;
    std::vector<string::view> _tokens;
};

//------------------------------------------------------------------------------
struct token
{
    char const* begin;
    char const* end;

    char operator[](std::size_t index) const {
        assert(begin + index < end);
        return begin[index];
    }

    template<std::size_t size> bool operator==(char const (&str)[size]) const {
        assert(begin < end);
        return strncmp(begin, str, std::max<std::ptrdiff_t>(end - begin, size - 1)) == 0;
    }

    template<std::size_t size> bool operator!=(char const (&str)[size]) const {
        assert(begin < end);
        return strncmp(begin, str, std::max<std::ptrdiff_t>(end - begin, size - 1)) != 0;
    }

    bool operator==(char ch) const {
        assert(begin < end);
        return *begin == ch && end == begin + 1;
    }

    bool operator!=(char ch) const {
        assert(begin < end);
        return *begin != ch || end != begin + 1;
    }
};

inline string::buffer operator+(char const* lhs, token const& rhs)
{
    return string::buffer(va("%s%.*s", lhs, static_cast<int>(rhs.end - rhs.begin), rhs.begin));
}

inline string::buffer operator+(string::buffer const& lhs, token const& rhs)
{
    return string::buffer(va("%s%.*s", lhs.c_str(), static_cast<int>(rhs.end - rhs.begin), rhs.begin));
}

//------------------------------------------------------------------------------
struct error
{
    token tok;
    string::buffer msg;
};

//------------------------------------------------------------------------------
template<typename T> using result = std::variant<error, T>;

//------------------------------------------------------------------------------
using tokenized = std::vector<token>;

//------------------------------------------------------------------------------
result<tokenized> tokenize(char const* begin, char const* end);

//------------------------------------------------------------------------------
class context
{
public:
    bool has_token() const {
        return _cursor < _tokens.data() + _tokens.size();
    }

    bool skip_token();

    template<std::size_t size> bool peek_token(char const (&string)[size]) const {
        return peek_token(string, string + size);
    }

    template<std::size_t size> bool check_token(char const (&string)[size]) {
        return check_token(string, string + size);
    }

    template<std::size_t size> bool expect_token(char const (&string)[size]) {
        return expect_token(string, string + size);
    }

    result<token> expect_any_token();

protected:
    string::buffer _filename;
    std::size_t _linenumber;

    tokenized _tokens;
    token* _cursor;

protected:
    bool peek_token(char const* begin, char const* end) const;
    bool check_token(char const* begin, char const* end);
    bool expect_token(char const* begin, char const* end);
};

} // namespace parser
