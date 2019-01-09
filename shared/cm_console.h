// cm_console.h
//

#pragma once

#include "cm_string.h"

#include <cstdlib>
#include <cstring>
#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace file {
class stream;
}

namespace parser {
class text;
}

namespace config {
class system;
}

class console;

//------------------------------------------------------------------------------
class console_buffer
{
public:
    console_buffer();

    void append(string::view text);
    void resize(std::size_t num_columns);
    void write(file::stream& steam) const;

    std::size_t num_rows() const { return _rows_begin - _rows_end; }
    char const* get_row(std::size_t index) const {
        std::size_t row = _rows[(_rows_begin - index - 1) & _rows_mask];
        return &_buffer[row & buffer_mask];
    }

protected:
    static constexpr std::size_t buffer_size = 1 << 16;
    static constexpr std::size_t buffer_mask = buffer_size - 1;

    std::array<char, buffer_size> _buffer;

    std::size_t _offset;

    std::vector<std::size_t> _rows;
    std::size_t _rows_mask; //!<
    std::size_t _rows_begin; //!< index of next row
    std::size_t _rows_end; //!< index of earliest row

    std::size_t _row_offset; //!< number of chars in the current row
    std::size_t _row_columns; //!< number of columns in the current row

    std::size_t _max_columns; //!< maximum number of columns in each row

protected:
    void append_row(string::view text);
    void append_endline(bool keep_color);

    std::size_t num_columns(string::view text) const;
    string::view advance_columns(std::size_t columns, string::view text) const;
};

//------------------------------------------------------------------------------
class console_input
{
public:
    console_input();

    void clear();
    void replace(string::view text);

    bool char_event(int key);
    bool key_event(int key, bool down);

    char const* begin() const { return _buffer.data(); }
    char const* end() const { return _buffer.data() + _length; }
    char const* cursor() const { return _buffer.data() + _cursor; }

protected:
    static constexpr std::size_t buffer_size = 256;
    std::array<char, buffer_size> _buffer;

    std::size_t _length;
    std::size_t _cursor;
    bool _control;
};

//------------------------------------------------------------------------------
class console_history
{
public:
    console_history();

    void insert(string::view text);
    std::size_t size() const { return std::min(_size, buffer_size); }
    string::view operator[](std::size_t index) const;

protected:
    constexpr static std::size_t element_size = 1 << 8;
    using element_type = std::array<char, element_size>;

    constexpr static std::size_t buffer_size = 1 << 8;
    constexpr static std::size_t buffer_mask = buffer_size - 1;
    std::array<element_type, buffer_size> _buffer;

    std::size_t _size;
};

//------------------------------------------------------------------------------
class console_command
{
public:
    using callback_type = std::function<void(parser::text const& args)>;

public:
    console_command(string::view name, callback_type callback);
    template<typename T>
    console_command(string::view name, T* obj, void(T::*callback)(parser::text const&))
        : console_command(name, [obj, callback](parser::text const& args){ (obj->*callback)(args); })
    {}

    void execute(parser::text const& args) const;

protected:
    friend console;

    string::buffer _name;
    callback_type _func;

    console_command* _next;
    static console_command* _head;
};

//------------------------------------------------------------------------------
class console
{
public:
    console();
    ~console();

    void printf(string::literal fmt, ...);
    void resize(std::size_t num_columns);
    bool char_event(int key);
    bool key_event(int key, bool down);

    float height() const { return _height; }
    std::size_t scroll() const { return _scroll_offset; }
    bool active() const { return _height > 0.f; }

    std::size_t num_rows() const { return _buffer.num_rows(); }
    char const* get_row(std::size_t index) const { return _buffer.get_row(index); }

    console_input const& input() const { return _input; }

protected:
    friend class console_command;

    struct insensitive_compare {
        using is_transparent = void;
        bool operator()(string::view lhs, string::view rhs) const {
            return stricmp(lhs, rhs) < 0;
        }
    };

    std::map<string::buffer, console_command*, insensitive_compare> _commands;

    console_command _command_set;
    console_command _command_list;
    console_command _command_list_vars;

    console_buffer _buffer;
    console_input _input;
    console_input _saved;
    console_history _history;

    float _height;
    std::size_t _scroll_offset;
    std::size_t _history_offset;
    bool _control;

    static console* _singleton;

protected:
    void execute(string::view text);
    static void command_set(parser::text const& args);
    static void command_list(parser::text const& args);
};
