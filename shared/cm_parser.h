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

    bool operator==(string::view str) const {
        assert(begin < end);
        return string::view{begin, end} == str;
    }

    bool operator!=(string::view str) const {
        assert(begin < end);
        return string::view{begin, end} != str;
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
result<tokenized> tokenize(string::view text);

//------------------------------------------------------------------------------
std::vector<string::view> split_lines(string::view text);

//------------------------------------------------------------------------------
class context
{
public:
    context() {}
    context(string::view text, string::view filename, std::size_t linenumber = 1);

    bool has_token() const {
        return _cursor < _tokens.data() + _tokens.size();
    }

    result<token> next_token();
    bool skip_token();
    bool skip_braced_section(bool parse_opening_brace = true);

    bool peek_token(string::view text) const;
    bool check_token(string::view text);
    bool expect_token(string::view text);

    bool has_error() const { return std::holds_alternative<error>(_error); }
    error get_error() const { return std::get<error>(_error); }
    void set_error(error e) { _error = e; }
    void clear_error() { _error = std::variant<error>{}; }

    struct token_info
    {
        string::view filename;
        std::size_t linenumber;
        std::size_t column;
    };

    token_info get_info(token token) const;
    string::view get_line(std::size_t number) const {
        return _lines[number - _linenumber];
    }

protected:
    string::buffer _filename;
    std::size_t _linenumber;

    string::view _text;
    std::vector<token> _tokens;
    std::vector<string::view> _lines;
    token* _cursor;
    std::variant<error> _error;

protected:
};

} // namespace parser
