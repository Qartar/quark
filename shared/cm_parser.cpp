// cm_parser.cpp
//

#include "cm_parser.h"
#include <cctype>
#include <cstring>

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

} // namespace parser
