// cm_parser.cpp
//

#include "cm_parser.h"
#include <cctype>
#include <cstring>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
namespace parser {

//------------------------------------------------------------------------------
/**
 * Rules for parsing commmand line arguments taken from MSDN:
 *
 * "Parsing C Command-Line Arguments"
 * https://msdn.microsoft.com/en-us/library/a1y7w461.aspx
 */
text::text(string::view view)
{
    char const* string = view.begin();
    char const* end = view.end();

    // Skip leading whitespace.
    while (isblank((unsigned char)*string)) {
        ++string;
    }

    _buffer.resize(end - string + 1);
    char* ptr = _buffer.data();

    while (string < end) {

        bool inside_quotes = false;
        char const* token = ptr;

        while (string < end) {
            // "Arguments are delimited by whitespace, which is either a space
            // or a tab."
            if (!inside_quotes && isblank((unsigned char)*string)) {
                break;
            }
            // "A string surrounded by double quotation marks is interpreted as
            // a single argument, regardless of white space contained within. A
            // quoted string can be embedded in an argument."
            else if (*string == '\"') {
                inside_quotes = !inside_quotes;
                ++string;
            }
            else if (*string == '\\') {

                // Count the number of consecutive backslashes.
                int ns = 1;
                while (*(string+ns) == '\\') {
                    ++ns;
                }

                // "A double quotation mark preceded by a backslash, \", is
                // interpreted as a literal double quotation mark (")."
                if (*(string+1) == '\"') {
                    ++string; // Skip escaping backslash.
                    *ptr++ = *string++;
                }
                // "Backslashes are interpreted literally, unless they
                // immediately precede a double quotation mark.
                else if (*(string+ns) != '\"') {
                    while (ns--) {
                        *ptr++ = *string++;
                    }
                }
                // "If an even number of backslashes is followed by a double
                // quotation mark, then one backslash (\) is placed in the argv
                // array for every pair of backslashes (\\), and the double
                // quotation mark (") is interpreted as a string delimiter."
                else if ((ns & 1) == 0) {
                    while ((ns -= 2) > -1) {
                        ++string; // Skip escaping backslash.
                        *ptr++ = *string++;
                    }
                    inside_quotes = !inside_quotes;
                    ++string; // Skip string delimiter.
                }
                // "If an odd number of backslashes is followed by a double
                // quotation mark, then one backslash (\) is placed in the argv
                // array for every pair of backslashes (\\) and the double
                // quotation mark is interpreted as an escape sequence by the
                // remaining backslash, causing a literal double quotation mark
                // (") to be placed in argv."
                else {
                    while ((ns -= 2) > -2) {
                        ++string; // Skip escaping backslash.
                        *ptr++ = *string++;
                    }
                    *ptr++ = *string++;
                }
            // Otherwise copy the character.
            } else {
                *ptr++ = *string++;
            }
        }

        // Append the token.
        _tokens.push_back({token, ptr});
        *ptr++ = '\0';

        // Skip trailing whitespace.
        while (isblank((unsigned char)*string)) {
            ++string;
        }
    }
}

//------------------------------------------------------------------------------
text::text(text&& other)
    : _buffer(std::move(other._buffer))
    , _tokens(std::move(other._tokens))
{}

//------------------------------------------------------------------------------
text& text::operator=(text&& other)
{
    std::swap(_buffer, other._buffer);
    std::swap(_tokens, other._tokens);
    return *this;
}

//------------------------------------------------------------------------------
result<tokenized> tokenize(string::view text)
{
    char const* str = text.begin();
    char const* end = text.end();

    std::vector<token> tokens;
    while (true) {
        // skip leading whitespace
        while (str < end && *str <= ' ') {
            assert(*str);
            ++str;
        }

        if (str >= end) {
            return tokens;
        }

        token t{str, str};
        switch (*str) {
            case '=':
            case '+':
            case '-':
            case '*':
            case '/':
            case '^':
            case '(':
            case ')':
            case ',':
            case '{':
            case '}':
            case ';':
                ++t.end;
                ++str;
                tokens.push_back(t);
                continue;
        }

        if (*str >= '0' && *str <= '9' || *str == '.') {
            bool has_dot = false;
            while (str < end && (*str >= '0' && *str <= '9' || (!has_dot && *str == '.'))) {
                if (*str == '.') {
                    if (has_dot) {
                        return error{{str, str+1}, "invalid literal"};
                    }
                    has_dot = true;
                }
                ++t.end;
                ++str;
            }
            tokens.push_back(t);
            continue;
        }

        if (*str >= 'a' && *str <= 'z' || *str >= 'A' && *str <= 'Z') {
            while (str < end && (*str == '_' || *str >= 'a' && *str <= 'z' || *str >= 'A' && *str <= 'Z')) {
                ++t.end;
                ++str;
            }
            tokens.push_back(t);
            continue;
        }

        return error{{str, str+1}, "invalid character '" + token{str, str+1} + "'"};
    }

    // never gets here
}

//------------------------------------------------------------------------------
std::vector<string::view> split_lines(string::view text)
{
    std::vector<string::view> lines;
    char const* begin = text.begin();
    char const* end = text.begin();

    while (end < text.end()) {
        while (*end != '\n') {
            ++end;
        }
        lines.push_back({begin, end});
        begin = ++end;
    }

    if (begin != end) {
        lines.push_back({begin, end});
    }

    return lines;
}

//------------------------------------------------------------------------------
context::context(string::view text, string::view filename, std::size_t linenumber)
    : _filename(filename)
    , _linenumber(linenumber)
    , _text(text)
    , _cursor(nullptr)
{
    auto tokens = tokenize(_text);
    if (std::holds_alternative<tokenized>(tokens)) {
        _tokens = std::move(std::get<tokenized>(tokens));
        _cursor = _tokens.data();
    }
    _lines = split_lines(_text);
}

//------------------------------------------------------------------------------
result<token> context::next_token()
{
    if (has_token()) {
        return *_cursor++;
    } else if (_tokens.size()) {
        return error{*(_cursor - 1), "expected token"};
    } else {
        return error{token{"", ""}, "expected token"};
    }
}

//------------------------------------------------------------------------------
result<token> context::peek_token() const
{
    if (has_token()) {
        return *_cursor;
    } else if (_tokens.size()) {
        return error{*(_cursor - 1), "expected token"};
    } else {
        return error{token{"", ""}, "expected token"};
    }
}

//------------------------------------------------------------------------------
bool context::skip_token()
{
    auto tok = next_token();
    if (std::holds_alternative<token>(tok)) {
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool context::skip_braced_section(bool parse_opening_brace)
{
    if (parse_opening_brace && !expect_token("{")) {
        return false;
    }

    for (std::size_t count = 1; count;) {
        if (check_token("}")) {
            --count;
        } else if (check_token("{")) {
            ++count;
        } else if (!skip_token()) {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
bool context::peek_token(string::view text) const
{
    if (has_token() && *_cursor == text) {
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool context::check_token(string::view text)
{
    if (has_token() && *_cursor == text) {
        ++_cursor;
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool context::expect_token(string::view text)
{
    auto tok = next_token();
    if (std::holds_alternative<token>(tok)) {
        if (std::get<token>(tok) == text) {
            return true;
        } else {
            set_error(error{std::get<token>(tok), "expected token '%s', found '%s'"});
            return false;
        }
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
context::token_info context::get_info(token token) const
{
    auto it = std::upper_bound(_lines.begin(), _lines.end(), token.begin,
        [](char const* begin, string::view rhs) {
            return begin < rhs.end();
        }
    );

    return {
        _filename,
        _linenumber + std::distance(_lines.begin(), it),
        1 + static_cast<std::size_t>(token.begin - it->begin()),
    };
}

} // namespace parser
