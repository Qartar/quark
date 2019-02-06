//  cm_config.cpp
//

#include "cm_config.h"
#include "cm_shared.h"
#include "cm_filesystem.h"
#include "cm_parser.h"

#include <Shlobj.h>
#include <PathCch.h>

////////////////////////////////////////////////////////////////////////////////
namespace config {

variable* variable::_head = nullptr;
system* system::_singleton = nullptr;

namespace {

static constexpr string_literal type_strings[] = { "string", "integer", "boolean", "scalar" };

//------------------------------------------------------------------------------
value_type type_from_string(string_view type_string)
{
    for (size_t ii = 0; ii < countof(type_strings); ++ii) {
        if (strcmp(type_string, type_strings[ii]) == 0) {
            return static_cast<value_type>(ii);
        }
    }
    return value_type::string;
}

} // anonymous namespace

//------------------------------------------------------------------------------
variable_base::variable_base(string_view name, string_view value, string_view type_string, string_view flags_string, string_view description)
    : _name(name)
    , _description(description)
    , _value(value)
    , _flags(atoi(flags_string.c_str()))
    , _type(type_from_string(type_string))
{}

//------------------------------------------------------------------------------
void variable_base::set(string_view value)
{
    char* end = nullptr;

    switch (type()) {
        case value_type::string: {
            set_string(value);
            break;
        }

        case value_type::integer: {
            long d = strtol(value.c_str(), &end, 0);
            if (end != value.begin()) {
                set_integer(d);
            } else {
                log::message("cannot set variable '^fff%s^xxx' to non-integer value '^fff%s^xxx'\n", name().c_str(), value.c_str());
            }
            break;
        }

        case value_type::boolean: {
            if (!stricmp(value, "true") || !strcmp(value, "1")) {
                set_boolean(true);
            } else if (!stricmp(value, "false") || !strcmp(value, "0")) {
                set_boolean(false);
            } else {
                log::message("cannot set variable '^fff%s^xxx' to non-boolean value '^fff%s^xxx'\n", name().c_str(), value.c_str());
            }
            break;
        }

        case value_type::scalar: {
            float f = strtof(value.c_str(), &end);
            if (end != value.begin()) {
                set_scalar(f);
            } else {
                log::message("cannot set variable '^fff%s^xxx' to non-scalar value '^fff%s^xxx'\n", name().c_str(), value.c_str());
            }
            break;
        }
    }
}

//------------------------------------------------------------------------------
void variable_base::set_string(string_view s)
{
    _value = string_buffer(s);
    _flags |= flags::modified;
}

//------------------------------------------------------------------------------
void variable_base::set_integer(int i)
{
    set_string(to_string(i));
}

//------------------------------------------------------------------------------
void variable_base::set_boolean(bool b)
{
    set_string(to_string(b));
}

//------------------------------------------------------------------------------
void variable_base::set_scalar(float f)
{
    set_string(to_string(f));
}

//------------------------------------------------------------------------------
string_buffer variable_base::to_string(int i) const
{
    return string_buffer(va("%d", i));
}

//------------------------------------------------------------------------------
string_buffer variable_base::to_string(bool b) const
{
    return string_buffer(b ? "true" : "false");
}

//------------------------------------------------------------------------------
string_buffer variable_base::to_string(float f) const
{
    string_buffer s(va("%f", f));
    if (s[0] != '.') {
        // strip trailing zeroes
        while (s.back() == '0') {
            s.pop_back();
        }
        // strip trailing decimal point
        if (s.back() == '.') {
            s.pop_back();
        }
    }
    return string_buffer(s.c_str());
}

//------------------------------------------------------------------------------
string_view variable_base::get_string() const
{
    return _value;
}

//------------------------------------------------------------------------------
int variable_base::get_integer() const
{
    return atoi(_value.c_str());
}

//------------------------------------------------------------------------------
bool variable_base::get_boolean() const
{
    return _value == "true" ? true
        : _value == "false" ? false
        : atoi(_value.c_str()) != 0; 
}

//------------------------------------------------------------------------------
float variable_base::get_scalar() const
{
    return static_cast<float>(std::atof(_value.c_str()));
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
variable::variable(string_view name, string_view value, value_type type, int flags, string_view description)
{
    if (system::_singleton) {
        _base = system::_singleton->create(name, value, type, flags, description);
        _next = nullptr;
    } else {
        _base = std::make_shared<variable_base>(name, value, type, flags, description);
        _next = _head;
        _head = this;
    }
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
string::operator string_view() const
{
    return get_string();
}

//------------------------------------------------------------------------------
config::string& string::operator=(string_view s)
{
    set_string(s);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
integer::operator int() const
{
    return get_integer();
}

//------------------------------------------------------------------------------
config::integer& integer::operator=(int i)
{
    set_integer(i);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
boolean::operator bool() const
{
    return get_boolean(); 
}

//------------------------------------------------------------------------------
config::boolean& boolean::operator=(bool b)
{
    set_boolean(b);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
scalar::operator float() const
{
    return get_scalar();
}

//------------------------------------------------------------------------------
config::scalar& scalar::operator=(float f)
{
    set_scalar(f);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
int get_config_path(char *path, std::size_t size, bool create = false)
{
    PWSTR pszPath;
    WCHAR wPath[1024];

    SHGetKnownFolderPath( FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &pszPath );

    PathCchCombine(wPath, countof(wPath), pszPath, L"Quark");

    if (create) {
        CreateDirectoryW(wPath, NULL);
    }

    PathCchAppend(wPath, countof(wPath), L"config.ini" );

    CoTaskMemFree( pszPath );

    return WideCharToMultiByte(
        CP_ACP,
        WC_NO_BEST_FIT_CHARS,
        wPath,
        -1,
        path,
        narrow_cast<int>(size),
        NULL,
        NULL);
}

//------------------------------------------------------------------------------
char const* system::print(variable_base const* base, int /*tab_size*/) const
{
    static char buffer[MAX_STRING];

    sprintf(buffer, "%-20s %-20s %-8s %3d \"%s\"\n",
         base->name().c_str(),
         va("\"%s\"", base->value().c_str()).c_str(),
         config::type_strings[static_cast<int>(base->type())].c_str(),
         base->flags(),
         base->description().c_str() );

    return buffer;
}

//------------------------------------------------------------------------------
system::system()
{
    assert(_singleton == nullptr);
    _singleton = this;
}

//------------------------------------------------------------------------------
system::~system()
{
    assert(_singleton == this);
    _singleton = nullptr;
}

//------------------------------------------------------------------------------
void system::init()
{
    // iterate over all variables initialized before the config system
    {
        for (auto var = variable::_head; var; var = var->_next) {
            auto it = _variables.find(var->_base->_name);
            if (it != _variables.end()) {
                assert(it->second != var->_base);
                assert(it->second->type() == var->type());
                // point all instances of the variable to the same base
                var->_base = it->second;
            } else {
                // add variable base to dictionary
                _variables[var->_base->_name] = var->_base;
            }
        }
        config::variable::_head = nullptr;
    }

    // load saved configuration from ini file
    {
        char filename[LONG_STRING];

        get_config_path(filename, countof(filename));
        file::buffer buffer = file::read(filename);

        char const* ptr = (char const*)buffer.data();
        char const* end = ptr + buffer.size();

        while(ptr && ptr < end) {
            char const* next = strchr(ptr, '\n');
            parser::text text(ptr, next);

            ptr = next ? next + 1 : nullptr;
            if (text.tokens().size() < 2) {
                break;
            }

            auto it = _variables.find(text.tokens()[0]);
            if (it != _variables.end()) {
                // update existing variable
                auto type = type_from_string(text.tokens()[2]);
                if (type != it->second->_type) {
                    log::warning("archived variable '^fff%s^xxx' has wrong type, reverting to default\n", text.tokens()[0].c_str());
                } else {
                    it->second->_value = string_buffer(text.tokens()[1]);
                }
            } else {
                // create new variable
                _variables[string_buffer(text.tokens()[0])] =
                    std::make_shared<variable_base>(
                        text.tokens()[0],
                        text.tokens()[1],
                        text.tokens()[2],
                        text.tokens()[3],
                        text.tokens()[4]);
            }
        }
    }
}

//------------------------------------------------------------------------------
void system::shutdown()
{
    char filename[LONG_STRING];

    if (get_config_path(filename, countof(filename), true)) {
        file::stream file = file::open(filename, file::mode::write);

        for (auto it : _variables) {
            if (it.second->flags() & config::archive) {
                string_view line{print(it.second.get(), 4)};
                file.write((byte const*)line.c_str(), strlen(line));
            }
        }
    }
}

//------------------------------------------------------------------------------
void system::command_list(parser::text const& /*args*/)
{
    for (auto it : _singleton->_variables) {
        log::message(_singleton->print(it.second.get(), 4));
    }
}

//------------------------------------------------------------------------------
variable_base* system::find(string_view name)
{
    auto it = _variables.find(name);
    if (it != _variables.end()) {
        return it->second.get();
    } else {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
variable_base* system::find(string_view name, value_type type)
{
    auto it = _variables.find(name);
    if (it != _variables.end() && it->second->type() == type) {
        return it->second.get();
    } else {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
std::shared_ptr<variable_base> system::create(string_view name, string_view value, value_type type, int flags, string_view description)
{
    auto it = _variables.find(name);
    if (it != _variables.end()) {
        return it->second;
    } else {
        auto base = std::make_shared<variable_base>(name, value, type, flags, description);
        _variables[string_buffer(name)] = base;
        return base;
    }
}

//------------------------------------------------------------------------------
string_view system::get(string_view name)
{
    auto it = _variables.find(name);
    if (it != _variables.end()) {
        return it->second->value();
    } else {
        return string_view{};
    }
}

//------------------------------------------------------------------------------
bool system::set(string_view name, string_view value)
{
    auto it = _variables.find(name);
    if (it != _variables.end()) {
        it->second->set(value);
        return true;
    } else {
        return false;
    }
}

} // namespace config
