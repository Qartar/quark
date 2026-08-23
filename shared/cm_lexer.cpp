// cm_lexer.cpp
//

#include "cm_lexer.h"
#include "cm_filesystem.h"
#include "cm_shared.h"

#include <cstdarg>
#include <cstdio>

////////////////////////////////////////////////////////////////////////////////
namespace {
string::literal token_type_string(lexer::token_type type)
{
    switch (type) {
        case lexer::token_type::name: return "name";
        case lexer::token_type::string: return "string";
        case lexer::token_type::number: return "number";
        case lexer::token_type::punctuation: return "punctuation";
    }
    __assume(false);
}
} // anonymous namespace

//------------------------------------------------------------------------------
lexer::lexer(string::view filename)
    : _filename(filename)
    , _linenumber(1)
    , _last_error{}
    , _next_token(0)
{
    file::stream s = file::open(filename, file::mode::read);
    _buffer.resize(s.size());
    s.read((file::byte*)_buffer.data(), s.size());
    _buffer[_buffer.length()] = 0;
    s.close();

    split_lines();

    tokenize();
}

//------------------------------------------------------------------------------
void lexer::print() const
{
    for (std::size_t ii = 0; ii < _tokens.size(); ++ii) {
        if (_tokens[ii].begin > _tokens[ii].whitespace) {
            log::message("%.*s", int(_tokens[ii].begin - _tokens[ii].whitespace), _tokens[ii].whitespace);
        }
        switch (_tokens[ii].type) {
            case lexer::token_type::name:
                log::message("^da6%.*s", int(_tokens[ii].end - _tokens[ii].begin), _tokens[ii].begin);
                break;
            case lexer::token_type::string:
                log::message("^d88%.*s", int(_tokens[ii].end - _tokens[ii].begin), _tokens[ii].begin);
                break;
            case lexer::token_type::number:
                log::message("^8dd%.*s", int(_tokens[ii].end - _tokens[ii].begin), _tokens[ii].begin);
                break;
            case lexer::token_type::punctuation:
                log::message("^dd8%.*s", int(_tokens[ii].end - _tokens[ii].begin), _tokens[ii].begin);
                break;
        }
    }
    if (_tokens.size() && _buffer.end() > _tokens.back().end) {
        log::message("^ff4%.*s", int(_buffer.end() - _tokens.back().end), _tokens.back().end);
    }
}

//------------------------------------------------------------------------------
bool lexer::has_token() const {
    return _next_token < _tokens.size();
}

//------------------------------------------------------------------------------
bool lexer::peek_token(string::view expected) const
{
    if (_next_token < _tokens.size() && _tokens[_next_token] == expected) {
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool lexer::peek_token_type(token_type expected_type) const
{
    if (_next_token < _tokens.size() && _tokens[_next_token].type == expected_type) {
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool lexer::check_token(string::view expected)
{
    if (_next_token < _tokens.size() && _tokens[_next_token] == expected) {
        ++_next_token;
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool lexer::expect_token(string::view expected)
{
    if (_next_token >= _tokens.size()) {
        set_error({_buffer.end(), _buffer.end(), _buffer.end()}, "expected token");
        return false;
    } else if (_tokens[_next_token] != expected) {
        set_error(_tokens[_next_token], "expected token '%.*s', found '%.*s'",
            int(expected.length()), expected.begin(),
            int(_tokens[_next_token].end - _tokens[_next_token].begin),
            _tokens[_next_token].begin);
        return false;
    } else {
        ++_next_token;
        return true;
    }
}

//------------------------------------------------------------------------------
bool lexer::expect_any_token(lexer::token& t)
{
    if (_next_token >= _tokens.size()) {
        set_error({_buffer.end(), _buffer.end(), _buffer.end()}, "expected token");
        return false;
    } else {
        t = _tokens[_next_token++];
        return true;
    }
}

//------------------------------------------------------------------------------
bool lexer::expect_token_type(lexer::token& t, token_type type)
{
    if (_next_token >= _tokens.size()) {
        set_error({_buffer.end(), _buffer.end(), _buffer.end()}, "expected token");
        return false;
    } else if (_tokens[_next_token].type != type) {
        set_error(_tokens[_next_token], "expected %s, found '%.*s'",
            token_type_string(type).c_str(),
            int(_tokens[_next_token].end - _tokens[_next_token].begin),
            _tokens[_next_token].begin);
        return false;
    } else {
        t = _tokens[_next_token++];
        return true;
    }
}

//------------------------------------------------------------------------------
bool lexer::parse(string::buffer& s)
{
    token t;
    if (expect_token_type(t, token_type::string)) {
        assert(*t.begin == '\"');
        assert(*(t.end - 1) == '\"');

        std::size_t len = 0;
        for (char const* ptr = t.begin + 1; ptr < t.end - 1; ++ptr) {
            if (*ptr == '\\') {
                switch (*++ptr) {
                    case '\'':
                    case '\"':
                    case '\\':
                        break;
                    default:
                        // invalid or unsupported escape sequence, copy as-is
                        assert(false); // should have caught this during tokenization
                        ++len;
                        break;
                }
            }
            ++len;
        }

        s.resize(len);
        char* out = s.data();
        for (char const* ptr = t.begin + 1; ptr < t.end - 1; ++ptr) {
            if (*ptr == '\\') {
                switch (*++ptr) {
                    case '\'':
                    case '\"':
                    case '\\':
                        *out++ = *ptr;
                        break;
                    default:
                        // invalid or unsupported escape sequence, copy as-is
                        *out++ = '\\';
                        *out++ = *ptr;
                        break;
                }
            } else {
                *out++ = *ptr;
            }
        }
        *out = '\0';
        return true;
    } else  {
        return false;
    }
}

//------------------------------------------------------------------------------
bool lexer::parse(int& i)
{
    token t;
    if (expect_token_type(t, token_type::number)) {
        return (_snscanf_s(t.begin, t.end - t.begin, "%d", &i) == 1);
    } else  {
        return false;
    }
}

//------------------------------------------------------------------------------
bool lexer::parse(float& f)
{
    token t;
    if (expect_token_type(t, token_type::number)) {
        return (_snscanf_s(t.begin, t.end - t.begin, "%f", &f) == 1);
    } else  {
        return false;
    }
}

//------------------------------------------------------------------------------
bool lexer::parse(double& d)
{
    token t;
    if (expect_token_type(t, token_type::number)) {
        return (_snscanf_s(t.begin, t.end - t.begin, "%lf", &d) == 1);
    } else  {
        return false;
    }
}

//------------------------------------------------------------------------------
bool lexer::parse(vec2& v)
{
    if (expect_token("(")
        && parse(v.x)
        && expect_token(",")
        && parse(v.y)
        && expect_token(")")) {
        return true;
    } else  {
        return false;
    }
}

//------------------------------------------------------------------------------
bool lexer::parse(vec3& v)
{
    if (expect_token("(")
        && parse(v.x)
        && expect_token(",")
        && parse(v.y)
        && expect_token(",")
        && parse(v.z)
        && expect_token(")")) {
        return true;
    } else  {
        return false;
    }
}

//------------------------------------------------------------------------------
bool lexer::locate(token const& t, location& l) const
{
    if (t.begin < _buffer.begin() || t.begin > _buffer.end()) {
        return false;
    }

    auto it = std::upper_bound(_lines.begin(), _lines.end(), t.begin,
        [](char const* begin, char const* rhs) {
            return begin < rhs;
        }
    );

    assert(it != _lines.begin());
    --it;

    l = {
        _filename,
        _linenumber + std::distance(_lines.begin(), it),
        1 + static_cast<std::size_t>(t.begin - *it),
    };
    return true;
}

//------------------------------------------------------------------------------
void lexer::set_error(token t, string::literal fmt, ...)
{
    // Try to locate token line/column within the source
    if (!locate(t, _last_error)) {
        _last_error.filename = _filename;
        _last_error.line = 0;
        _last_error.column = 0;
    }

    // Concatenate filename, line/column if available, and error message
    int location_len = _last_error.column ? _scprintf("(%zu:%zu): ", _last_error.line, _last_error.column)
        : _last_error.line ? _scprintf("(%zu): ", _last_error.line) : 2;

    va_list ap;

    va_start(ap, fmt);
    int message_len = _vscprintf(fmt.c_str(), ap);
    va_end(ap);

    _last_error.message.resize(_filename.length() + location_len + message_len);
    strncpy_s(_last_error.message.data(), _last_error.message.length() + 1, _filename.begin(), _filename.length());

    if (_last_error.column) {
        _snprintf_s(
            _last_error.message.data() + _filename.length(),
            _last_error.message.length() + 1 - _filename.length(),
            location_len,
            "(%zu:%zu): ", _last_error.line, _last_error.column);
    } else if (_last_error.line) {
        _snprintf_s(
            _last_error.message.data() + _filename.length(),
            _last_error.message.length() + 1 - _filename.length(),
            location_len,
            "(%zu): ", _last_error.line);
    } else {
        strncpy_s(
            _last_error.message.data() + _filename.length(),
            _last_error.message.length() + 1 - _filename.length(),
            ": ",
            2);
    }

    va_start(ap, fmt);
    _vsnprintf_s(
        _last_error.message.data() + _filename.length() + location_len,
        _last_error.message.length() + 1 - _filename.length() - location_len,
        message_len,
        fmt.c_str(), ap);
    va_end(ap);
}

//------------------------------------------------------------------------------
void lexer::split_lines()
{
    assert(_buffer.begin());

    char const* begin = _buffer.begin();
    char const* end = begin;

    while (end = strstr(end, "\n")) {
        _lines.push_back(begin);
        begin = ++end;
    }

    if (begin != end) {
        _lines.push_back(begin);
    }
}

//------------------------------------------------------------------------------
bool lexer::tokenize()
{
    assert(_buffer.begin());
    assert(_buffer.end());

    char const* str = _buffer.begin();
    char const* end = _buffer.end();

    while (true) {
        char const* whitespace = str;

        // skip leading whitespace
        while (str < end && *str <= ' ') {
            assert(*str);
            ++str;
        }

        if (str >= end) {
            return true;
        }

        // skip comments
        if (str + 1 < end && str[0] == '/') {
            if (str[1] == '/') {
                while (str < end && *str != '\n') {
                    ++str;
                }
                continue;
            } else if (str[1] == '*') {
                char const* ptr = str;
                str += 2;
                while (true) {
                    if (str + 1 >= end) {
                        set_error({whitespace, ptr, ptr + 2}, "unexpected end of file in comment");
                        return false;
                    } else if (str[0] == '*' && str[1] == '/') {
                        str += 2;
                        break;
                    } else {
                        ++str;
                    }
                }
                continue;
            }
        }

        // evaluate name token
        char const* begin = str;
        // names can start with number digits
        if (*str >= '0' && *str <= '9') {
            while (str < end && (*str >= '0' && *str <= '9')) {
                ++str;
            }
            if (!(str < end && (*str == '_' || *str >= '0' && *str <= '9' || *str >= 'a' && *str <= 'z' || *str >= 'A' && *str <= 'Z'))) {
                str = begin;
            }
            // fall through
        }

        if (*str == '_' || *str >= 'a' && *str <= 'z' || *str >= 'A' && *str <= 'Z') {
            while (str < end && (*str == '_' || *str >= '0' && *str <= '9' || *str >= 'a' && *str <= 'z' || *str >= 'A' && *str <= 'Z')) {
                ++str;
            }
            _tokens.push_back({whitespace, begin, str, token_type::name});
            continue;
        }

        // evaluate number token
        if ((*str >= '0' && *str <= '9') || *str == '.'
            || (*str == '-' && str + 1 < end && ((str[1] >= '0' && str[1] <= '9') || str[1] =='.'))) {
            bool has_dot = false;
            if (*str == '-') {
                ++str;
            }
            while (str < end && (*str >= '0' && *str <= '9' || (!has_dot && *str == '.'))) {
                if (*str == '.') {
                    if (has_dot) {
                        set_error({whitespace, begin, str}, "invalid literal");
                        return false;
                    }
                    has_dot = true;
                }
                ++str;
            }
            _tokens.push_back({whitespace, begin, str, token_type::number});
            continue;
        }

        // evaluate punctuation token
        switch (*str) {
            case '=':
            case '+':
            case '-':
            case '*':
            case '/':
            case '^':
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case ':':
            case ';':
            case ',':
            case '|':
                ++str;
                _tokens.push_back({whitespace, begin, str, token_type::punctuation});
                continue;
        }

        // evaluate string token
        if (*str == '\"') {
            while (true) {
                if (++str >= end) {
                    set_error({whitespace, begin, str}, "unexpected end of file in string literal");
                    return false;
                } else if (*str == '\\') {
                    switch (*++str) {
                        // Only a small set of escape sequences are allowed, cf. parse(string::buffer&)
                        case '\'':
                        case '\"':
                        case '\\':
                            break;
                        default:
                            set_error({whitespace, begin, str}, "invalid escape sequence in string literal");
                            return false;
                    }
                } else if (*str == '\"') {
                    ++str;
                    break;
                }
            }
            _tokens.push_back({whitespace, begin, str, token_type::string});
            continue;
        }

        set_error({str, str, str+1}, "invalid character '%c'", *str);
        return false;
    }

    // never gets here
}
