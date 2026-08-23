// cm_lexer.h
//

#pragma once

#include "cm_string.h"
#include "cm_vector.h"
#include <vector>

////////////////////////////////////////////////////////////////////////////////
class lexer
{
public:
    struct location {
        string::view filename;
        std::size_t line;
        std::size_t column;
    };

    struct error : location {
        string::buffer message;
    };

    enum class token_type {
        name, //!< Alpha-numeric identifier
        string, //!< String literal delimited by double quotes, e.g. "Hello"
        number, //!< Numeric token, either integer or decimal
        punctuation, //!< Other special characters
    };

    struct token {
        char const* whitespace; //!< Beginning of the token including leading whitespace
        char const* begin; //!< Beginning of the token, skipping whitespace
        char const* end; //!< End of the token
        token_type type;

        //! Implicit conversion to string::view
        operator string::view() const {
            return string::view(begin, end);
        }
    };

public:
    lexer(string::view filename);

    //! Returns true if there have been any errors.
    bool has_error() const { return _last_error.message.length() > 0; }
    //! Set error state at the given token with the given formatted message
    void set_error(token t, string::literal fmt, ...);
    //! Returns the most recent error.
    error last_error() const { return _last_error; }

    //! Populates location data for the given token if it can be located and returns true.
    bool locate(token const& t, location& l) const;

    //! Returns true if there are any remaining tokens.
    bool has_token() const;

    //! Returns true if the next token matches the given string, does not advance the cursor.
    bool peek_token(string::view expected) const;

    //! Returns true if the next token type matches the given type, does not advance the cursor.
    bool peek_token_type(token_type expected_type) const;

    //! Returns true and advances the cursor if the next token matches the given string.
    bool check_token(string::view expected);

    //! Returns true and advances the cursor if the next token matches the given
    //! string. If there are no remaining tokens or the next token does not match
    //! the given string an error is generated.
    bool expect_token(string::view expected);

    //! Returns true, populates the given token, and advances the cursor if there
    //! are any remaining tokens.
    bool expect_any_token(token& t);

    //! Returns true, populates the given token, and advances the cursor if the
    //! next token has the given token type.
    bool expect_token_type(token& t, token_type type);

    //! Parse the next token as a string. Token must be type `string`.
    bool parse(string::buffer& s);

    //! Parse the next token as an integer. Token must be type `number`.
    bool parse(int& i);

    //! Parse the next token as a single-precision float. Token must be type `number`.
    bool parse(float& f);

    //! Parse the next token as a double-precision float. Token must be type `number`.
    bool parse(double& d);

    //! Parse the next five tokens as a vec2. Tokens must be `(` `number` `,` `number` `)`
    bool parse(vec2& v);

    //! Parse the next seven tokens as a vec3. Tokens must be `(` `number` `,` `number` `,` `number` `)`
    bool parse(vec3& v);

protected:
    //! Source filename
    string::buffer _filename;
    //! Starting line number (typically 1)
    std::size_t _linenumber;

    //! String buffer for tokens to point into
    string::buffer _buffer;
    //! Pointer into buffer at the start of each line
    std::vector<char const*> _lines;
    //! All tokens, points into _buffer
    std::vector<token> _tokens;

    //! Most recent error
    error _last_error;
    //! Next token to be parsed/consumed
    std::size_t _next_token;

protected:
    //! Splits the source text into lines, used for locating token line/column
    void split_lines();

    //! Splits the source text into discrete tokens
    bool tokenize();
};
