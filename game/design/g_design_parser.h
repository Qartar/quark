// g_design_parser.h
//

#include "cm_lexer.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
template<typename T> bool units_none(lexer&, T&) { /* no-op */ return true; }

//------------------------------------------------------------------------------
template<typename T> bool units_length(lexer& lex, T&)
{
    if (lex.check_token("m")) {
        // no-op
    } else {
        // no-op
    }
    return true;
}

//------------------------------------------------------------------------------
template<typename T> bool units_angle(lexer& lex, T& value)
{
    if (lex.check_token("deg")) {
        value *= T(math::deg2rad(1.0));
    } else if (lex.check_token("rad")) {
        // no-op
    } else {
        value *= T(math::deg2rad(1.0));
    }
    return true;
}

//------------------------------------------------------------------------------
template<typename T> bool units_mass(lexer& lex, T& value)
{
    if (lex.check_token("t")) {
        value *= T(1e3);
    } else if (lex.check_token("kg")) {
        // no-op
    } else {
        value *= T(1e3);
    }
    return true;
}

//------------------------------------------------------------------------------
template<typename T> bool units_speed(lexer& lex, T& value)
{
    if (lex.check_token("kn")) {
        value *= T(0.5144447);
    } else if (lex.check_token("m")
        && lex.check_token("/")
        && lex.check_token("s")) {
        // no-op
    } else {
        value *= T(0.5144447);
    }
    return true;
}

//------------------------------------------------------------------------------
template<typename T> bool units_power(lexer& lex, T& value)
{
    if (lex.check_token("shp")) {
        value *= T(0.7456999);
    } else {
        value *= T(0.7456999);
    }
    return true;
}

//------------------------------------------------------------------------------
template<typename T> bool units_time(lexer& lex, T&)
{
    if (lex.check_token("s")) {
        // no-op
    } else {
        // no-op
    }
    return true;
}

//------------------------------------------------------------------------------
template<typename T>
bool check_field(lexer& lex,
                 string::view field_name,
                 T& field_value,
                 bool (*check_units)(lexer&,T&) = units_none)
{
    if (lex.check_token(field_name)
        && lex.expect_token("=")
        && lex.parse(field_value)
        && check_units(lex, field_value)
        && lex.expect_token(";")) {
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
inline bool check_field(lexer& lex,
                 string::view field_name,
                 time_delta& field_value,
                 bool (*check_units)(lexer&,double&) = units_none<double>)
{
    double value;
    if (lex.check_token(field_name)
        && lex.expect_token("=")
        && lex.parse(value)
        && check_units(lex, value)
        && lex.expect_token(";")) {
        field_value = time_delta::from_seconds(value);
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
inline string::buffer make_literal(string::view s)
{
    std::size_t len = 2;
    for (char const* ptr = s.begin(); ptr < s.end(); ++ptr) {
        switch (*ptr) {
            case '\'':
            case '\"':
            case '\\':
                len += 2;
                break;
            default:
                len += 1;
                break;
        }
    }
    string::buffer b;
    b.resize(len);
    char* out = b.data();
    *out++ = '\"';
    for (char const* ptr = s.begin(); ptr < s.end(); ++ptr) {
        switch (*ptr) {
            case '\'':
            case '\"':
            case '\\':
                *out++ = '\\';
                *out++ = *ptr;
                break;
            default:
                *out++ = *ptr;
                break;
        }
    }
    *out++ = '\"';
    *out++ = '\0';
    return b;
}

} // namespace game
