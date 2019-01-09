// cm_config.h
//

#pragma once

#include "cm_shared.h"
#include "cm_string.h"

#include <memory>
#include <map>

////////////////////////////////////////////////////////////////////////////////
namespace parser {
class text;
}

//------------------------------------------------------------------------------
namespace config {

class system;
class variable;

using string_view = ::string::view;
using string_literal = ::string::literal;
using string_buffer = ::string::buffer;

using ::string::strcpy;
using ::string::sscanf;
using ::string::strlen;
using ::string::strncpy;

enum flags
{
    archive     = 1 << 0,
    server      = 1 << 1,
    reset       = 1 << 2,
    modified    = 1 << 3,
};

enum class value_type { string, integer, boolean, scalar };

//------------------------------------------------------------------------------
class variable_base
{
public:
    variable_base(string_view name, string_view value, value_type type, int flags, string_view description)
        : _name(name)
        , _description(description)
        , _value(value)
        , _flags(flags)
        , _type(type)
    {}
    variable_base(string_view name, string_view value, string_view type_string, string_view flags_string, string_view description);

    string_view name() const { return _name; }
    string_view description() const { return _description; }
    string_view value() const { return _value; }
    int flags() const { return _flags; }
    value_type type() const { return _type; }
    bool modified() const { return _flags & flags::modified; }
    void reset() { _flags &= ~flags::modified; }
    void set(string_view value);

protected:
    friend system;
    friend variable;

    string_buffer _name;
    string_buffer _description;
    string_buffer _value;
    int _flags;
    value_type _type;

protected:
    string_view get_string() const;
    int get_integer() const;
    bool get_boolean() const;
    float get_scalar() const;

    void set_string(string_view s);
    void set_integer(int i);
    void set_boolean(bool b);
    void set_scalar(float f);

    string_buffer to_string(int i) const;
    string_buffer to_string(bool b) const;
    string_buffer to_string(float f) const;
};

//------------------------------------------------------------------------------
class variable
{
public:
    string_view name() const { return _base->name(); }
    string_view description() const { return _base->description(); }
    string_view value() const { return _base->value(); }
    int flags() const { return _base->flags(); }
    value_type type() const { return _base->type(); }
    bool modified() const { return _base->modified(); }
    void reset() { _base->reset(); }
    void set(string_view value) { _base->set(value); }

protected:
    friend system;

    std::shared_ptr<variable_base> _base;

    variable* _next;
    static variable* _head;

protected:
    variable(string_view name, string_view value, value_type type, int flags, string_view description);

    string_view get_string() const { return _base->get_string(); }
    int get_integer() const { return _base->get_integer(); }
    bool get_boolean() const { return _base->get_boolean(); }
    float get_scalar() const { return _base->get_scalar(); }

    void set_string(string_view s) { _base->set_string(s); }
    void set_integer(int i) { _base->set_integer(i); }
    void set_boolean(bool b) { _base->set_boolean(b); }
    void set_scalar(float f) { _base->set_scalar(f); }

    string_buffer to_string(int i) const { return _base->to_string(i); }
    string_buffer to_string(bool b) const { return _base->to_string(b); }
    string_buffer to_string(float f) const { return _base->to_string(f); }
};

//------------------------------------------------------------------------------
class string : public variable
{
public:
    string(string_view name, string_view value, int flags, string_view description)
        : variable(name, value, value_type::string, flags, description)
    {}

    operator string_view() const;
    config::string& operator=(string_view s);
};

//------------------------------------------------------------------------------
class integer : public variable
{
public:
    integer(string_view name, int value, int flags, string_view description)
        : variable(name, to_string(value), value_type::integer, flags, description)
    {}

    operator int() const;
    config::integer& operator=(int i);
};

//------------------------------------------------------------------------------
class boolean : public variable
{
public:
    boolean(string_view name, bool value, int flags, string_view description)
        : variable(name, to_string(value), value_type::boolean, flags, description)
    {}

    operator bool() const;
    config::boolean& operator=(bool b);
};

//------------------------------------------------------------------------------
class scalar : public variable
{
public:
    scalar(string_view name, float value, int flags, string_view description)
        : variable(name, to_string(value), value_type::scalar, flags, description)
    {}

    operator float() const;
    config::scalar& operator=(float f);
};

//------------------------------------------------------------------------------
class system
{
public:
    system();
    ~system();

    void init();
    void shutdown();

    variable_base* find(string_view name);
    string_view get(string_view name);
    bool set(string_view name, string_view value);

    static void command_list(parser::text const& args);

    static system* singleton() { return _singleton; }

protected:
    friend variable;

    struct insensitive_compare {
        using is_transparent = void;
        bool operator()(string_view lhs, string_view rhs) const {
            return stricmp(lhs, rhs) < 0;
        }
    };

    std::map<string_buffer, std::shared_ptr<variable_base>, insensitive_compare> _variables;

    static system* _singleton;

protected:
    variable_base* find(string_view name, value_type type);
    char const* print(variable_base const* base, int tab_size = 4) const;

    std::shared_ptr<variable_base> create(string_view name, string_view value, value_type type, int flags, string_view description);
};

} // namespace config
