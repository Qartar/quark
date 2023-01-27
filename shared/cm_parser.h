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

    //! Return true if there are any remaining tokens
    bool has_token() const {
        return _cursor < _tokens.data() + _tokens.size();
    }

    //! Return the next token and advance the cursor
    result<token> next_token();
    //! Return the next token without advancing the cursor
    result<token> peek_token() const;
    //! Skip the next token
    bool skip_token();
    //! Skip all tokens up to and including the matching closing brace
    bool skip_braced_section(bool parse_opening_brace = true);
    //! Skip all tokens on the same line as the previous token
    bool skip_rest_of_line();

    //! Returns true if the next token matches the given string
    bool peek_token(string::view text) const;
    //! Returns true and advances the cursor if the next token matches the given string
    bool check_token(string::view text);
    //! Returns true and advances the cursor if the next token matches the given string
    //! If the next token does not match the given string an error is generated
    bool expect_token(string::view text);

    bool has_error() const { return !_errors.empty(); }
    error get_error() const { assert(has_error()); return _errors.back(); }
    error set_error(error e) { _errors.resize(1); _errors[0] = e; return e; }
    void clear_error() { _errors.clear(); }

    struct token_info
    {
        string::view filename;
        std::size_t linenumber;
        std::size_t column;
    };

    //! Return filename, line number, and column of first character for the given token
    token_info get_info(token token) const;
    //! Return the full line of text at the given line number
    string::view get_line(std::size_t number) const {
        return _lines[number - _linenumber];
    }

protected:
    string::buffer _filename; //!< filename of the text
    std::size_t _linenumber; //!< first line number of the text, typically 1

    string::view _text; //!< text to be parsed
    std::vector<token> _tokens; //!< all tokens in the text
    std::vector<string::view> _lines; //!< all lines in the text
    token* _cursor; //!< next unparsed token
    std::vector<error> _errors;
};

} // namespace parser
