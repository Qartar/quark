// cm_console.cpp
//

#include "cm_console.h"
#include "cm_config.h"
#include "cm_filesystem.h"
#include "cm_parser.h"
#include "cm_keys.h"
#include "cm_shared.h"

#include <cassert>
#include <cstdarg>


////////////////////////////////////////////////////////////////////////////////
console_buffer::console_buffer()
    : _offset(0)
    , _row_offset(0)
    , _row_columns(0)
{
    resize(100);
}

//------------------------------------------------------------------------------
void console_buffer::resize(std::size_t num_columns)
{
    _max_columns = num_columns;
    _rows.resize(1LL << std::ilogb(buffer_size / num_columns));
    _rows_mask = _rows.size() - 1;
    _rows_begin = 0;
    _rows_end = 0;
    _rows[0] = _offset;
}

//------------------------------------------------------------------------------
void console_buffer::write(file::stream& stream) const
{
    for (std::size_t row = _rows_end; row <= _rows_begin; ++row) {
        stream.printf("%s\r\n", _buffer.data() + (_rows[row & _rows_mask] & buffer_mask));
    }
}

//------------------------------------------------------------------------------
void console_buffer::append(string::view text)
{
    char const* row_begin = text.begin();
    while (row_begin < text.end()) {
        char const* row_end = strpbrk(row_begin, "\r\n\t");
        if (!row_end) {
            row_end = text.end();
        }

        // row would wrap across the end of the buffer
        std::size_t remaining = buffer_size - (_offset & buffer_mask);
        if (std::size_t(row_end - row_begin) >= remaining) {
            // copy current row to beginning of buffer
            if (_row_offset) {
                memcpy(_buffer.data(), _buffer.data() + (_offset & buffer_mask) - _row_offset, _row_offset);
                _rows[_rows_begin & _rows_mask] = _offset + remaining;
            }
            _offset += remaining;
        }

        switch (*row_begin) {
            case '\r':
                row_begin++;
                break;

            case '\n':
                append_endline(false);
                row_begin++;
                break;

            case '\t':
                row_begin++;
                break;

            default:
                break;
        }

        if (row_begin < row_end) {
            append_row({row_begin, row_end});
            row_begin = row_end;
        }
    }

    _buffer[_offset & buffer_mask] = '\0';

    if (_rows_begin - _rows_end >= _rows.size()) {
        _rows_end = _rows_begin - (_rows.size() - 1);
    }

    while (_rows[_rows_begin & _rows_mask] - _rows[_rows_end & _rows_mask] >= buffer_size) {
        ++_rows_end;
    }
}

//------------------------------------------------------------------------------
void console_buffer::append_row(string::view text)
{
    char const* row_begin = text.begin();
    char const* row_end = text.end();
    while (row_begin < text.end()) {
        std::size_t row_columns = num_columns({row_begin, row_end});

        if (_row_columns == _max_columns) {
            append_endline(true);
        }

        // word wrap
        if (row_columns + _row_columns > _max_columns) {
            char const* prev = row_begin;

            while (prev < text.end() && isspace(*prev)) {
                ++prev;
            }

            char const* next = strpbrk(prev, "\r\n\t ");
            if (next >= text.end()) {
                next = text.end();
            }
            std::size_t next_columns = num_columns({row_begin, next});

            if (next_columns > _max_columns) {
                // print as many characters as possible on the current row
                row_columns = _max_columns - _row_columns;
                row_end = advance_columns(row_columns, {row_begin, text.end()}).begin();
            } else if (next_columns + _row_columns > _max_columns) {
                append_endline(true);
                while (row_begin < text.end() && isspace(*row_begin)) {
                    ++row_begin;
                }
                continue;
            } else {
                // print one word at a time until we get to the end of the row
                row_columns = next_columns;
                row_end = next;
            }
        }

        // write text to row
        memcpy(_buffer.data() + (_offset & buffer_mask), row_begin, row_end - row_begin);
        _offset += row_end - row_begin;
        _row_offset += row_end - row_begin;
        _row_columns += row_columns;
        row_begin = row_end;
        row_end = text.end();
    }
}

//------------------------------------------------------------------------------
void console_buffer::append_endline(bool keep_color)
{
    _buffer[_offset % buffer_size] = '\0';
    _offset += 1;
    _rows_begin += 1;
    _rows[_rows_begin & _rows_mask] = _offset;
    _row_offset = 0;
    _row_columns = 0;

    if (keep_color) {
        std::size_t prev_row = _rows[(_rows_begin - 1) & _rows_mask];
        char const* s = &_buffer[prev_row & buffer_mask];

        char const* prev_color = nullptr;
        while ((s = strchr(s, '^'))) {
            if (is_color(s)) {
                prev_color = s;
            }
            ++s;
        }

        if (prev_color && prev_color[1] != 'x') {
            append_row({prev_color, prev_color + 4});
        }
    }
}

//------------------------------------------------------------------------------
std::size_t console_buffer::num_columns(string::view text) const
{
    std::size_t len = 0;
    char const* prev = text.begin();
    while (char const* next = find_color(prev, text.end())) {
        len += next - prev;
        prev = next + 4;
    }
    len += text.end() - prev;
    return len;
}

//------------------------------------------------------------------------------
string::view console_buffer::advance_columns(std::size_t columns, string::view text) const
{
    std::size_t len = 0;
    char const* prev = text.begin();
    while (char const* next = find_color(prev, text.end())) {
        if (next - prev >= ptrdiff_t(columns - len)) {
            break;
        }
        len += next - prev;
        prev = next + 4;
    }
    if (text.end() - prev >= ptrdiff_t(columns - len)) {
        return {text.end() - (text.end() - prev) + (columns - len), text.end()};
    } else {
        return {text.end(), text.end()};
    }
}

//------------------------------------------------------------------------------
console_input::console_input()
    : _buffer{}
    , _length(0)
    , _cursor(0)
    , _control(false)
{}

//------------------------------------------------------------------------------
void console_input::clear()
{
    _buffer[0] = '\0';
    _length = 0;
    _cursor = 0;
    _control = false;
}

//------------------------------------------------------------------------------
void console_input::replace(string::view text)
{
    strncpy(_buffer.data(), text, buffer_size);
    _length = text.length();
    _cursor = _length;
}

//------------------------------------------------------------------------------
bool console_input::char_event(int key)
{
    if (key >= K_SPACE && key < K_BACKSPACE) {
        if (_length < buffer_size - 1) {
            memmove(_buffer.data() + _cursor + 1,
                    _buffer.data() + _cursor,
                    _length - _cursor);
            _buffer[_cursor++] = (char)key;
            _buffer[++_length] = '\0';
        }
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool console_input::key_event(int key, bool down)
{
    bool consumed = false;

    if (key == K_CTRL) {
        _control = down;
    }

    if (!down) {
        return consumed;
    }

    if (key == K_LEFTARROW) {
        if (_control) {
            while (_cursor > 0 && isspace(_buffer[_cursor - 1])) {
                --_cursor;
            }
            while (_cursor > 0 && !isspace(_buffer[_cursor - 1])) {
                --_cursor;
            }
        } else if (_cursor > 0) {
            --_cursor;
        }
        consumed = true;
    } else if (key == K_RIGHTARROW) {
        if (_control) {
            while (_cursor < _length && isspace(_buffer[_cursor])) {
                ++_cursor;
            }
            while (_cursor < _length && !isspace(_buffer[_cursor])) {
                ++_cursor;
            }
        } else if (_cursor < _length) {
            ++_cursor;
        }
        consumed = true;
    } else if (key == K_DEL) {
        std::size_t len = 0;
        if (_control && isspace(_buffer[_cursor])) {
            while (_cursor + len < _length && isspace(_buffer[_cursor + len])) {
                ++len;
            }
        } else if (_control && !isspace(_buffer[_cursor])) {
            while (_cursor + len < _length && !isspace(_buffer[_cursor + len])) {
                ++len;
            }
        } else if (!_control && _cursor < _length) {
            ++len;
        }
        if (len) {
            memmove(_buffer.data() + _cursor,
                    _buffer.data() + _cursor + len,
                    _length + len - _cursor);
            _length -= len;
        }
        consumed = true;
    } else if (key == K_BACKSPACE) {
        std::size_t len = 0;
        if (_control && _cursor > 0 && isspace(_buffer[_cursor - 1])) {
            while (_cursor > len && isspace(_buffer[_cursor - len - 1])) {
                ++len;
            }
        } else if (_control && _cursor > 0 && !isspace(_buffer[_cursor - 1])) {
            while (_cursor > len && !isspace(_buffer[_cursor - len - 1])) {
                ++len;
            }
        } else if (!_control && _cursor > 0) {
            ++len;
        }
        if (len) {
            memmove(_buffer.data() + _cursor - len,
                    _buffer.data() + _cursor,
                    _length - _cursor + len);
            _cursor -= len;
            _length -= len;
        }
        consumed = true;
    } else if (key == K_HOME) {
        _cursor = 0;
        consumed = true;
    } else if (key == K_END) {
        _cursor = _length;
        consumed = true;
    }

    assert(_cursor <= _length);
    return consumed;
}

////////////////////////////////////////////////////////////////////////////////
console_history::console_history()
    : _size(0)
{}

//------------------------------------------------------------------------------
void console_history::insert(string::view text)
{
    strncpy(_buffer[_size++ & buffer_mask].data(), text, element_size);
}

//------------------------------------------------------------------------------
string::view console_history::operator[](std::size_t index) const
{
    assert(index < _size);
    return string::view(_buffer[(_size - index - 1) & buffer_mask].data());
}

////////////////////////////////////////////////////////////////////////////////
console_command* console_command::_head = nullptr;

//------------------------------------------------------------------------------
console_command::console_command(string::view name, callback_type callback)
    : _name(name)
    , _func(callback)
    , _next(_head)
{
    if (console::_singleton) {
        assert(_head == nullptr);
        console::_singleton->_commands[_name] = this;
    } else {
        _head = this;
    }
}

//------------------------------------------------------------------------------
void console_command::execute(parser::text const& args) const
{
    if (_func) {
        _func(args);
    }
}

////////////////////////////////////////////////////////////////////////////////
console* console::_singleton = nullptr;

//------------------------------------------------------------------------------
console::console()
    : _command_set("set", &console::command_set)
    , _command_list("listCmds", &console::command_list)
    , _command_list_vars("listVars", &config::system::command_list)
    , _height(0)
    , _scroll_offset(0)
    , _history_offset(0)
    , _control(false)
{
    assert(_singleton == nullptr);
    _singleton = this;

    console_command* command = console_command::_head;
    while (command) {
        _commands[command->_name] = command;
        command = command->_next;
    }
    console_command::_head = nullptr;
}

//------------------------------------------------------------------------------
console::~console()
{
    assert(_singleton == this);
    _singleton = nullptr;
}

//------------------------------------------------------------------------------
void console::resize(std::size_t num_columns)
{
    _buffer.resize(num_columns);
}

//------------------------------------------------------------------------------
void console::printf(string::literal fmt, ...)
{
    constexpr int msg_size = 1024;
    char msg[msg_size];
    va_list ap;

    va_start(ap, fmt);
    int len = vsnprintf(msg, msg_size, fmt.c_str(), ap);
    va_end(ap);

    if (len > 0) {
        _buffer.append({msg, msg + len});
    }
}

//------------------------------------------------------------------------------
bool console::char_event(int key)
{
    if (key == '`') {
        if (_height) {
            _height = .0f;
        } else {
            _height = .5f;
        }
        return true;
    } else if (!active()) {
        return false;
    } else if (key == K_ENTER) {
        log::message("]%s\n", _input.begin());
        _history.insert({_input.begin(), _input.end()});
        _history_offset = 0;
        execute({_input.begin(), _input.end()});
        _input.clear();
        return true;
    } else {
        return _input.char_event(key);
    }
}

//------------------------------------------------------------------------------
bool console::key_event(int key, bool down)
{
    if (key == K_CTRL) {
        _control = down;
    }

    if (!active()) {
        return false;
    } else if (down && key == K_UPARROW) {
        if (_history_offset < _history.size()) {
            if (!_history_offset++) {
                _saved = _input;
            }
            _input.replace(_history[_history_offset - 1]);
        }
        return true;
    } else if (down && key == K_DOWNARROW) {
        if (_history_offset) {
            if (--_history_offset) {
                _input.replace(_history[_history_offset - 1]);
            } else {
                _input.replace({_saved.begin(), _saved.end()});
            }
        }
        return true;
    } else if (down && _control && key == K_HOME) {
        _scroll_offset = _buffer.num_rows();
        return true;
    } else if (down && _control && key == K_END) {
        _scroll_offset = 0;
        return true;
    } else if (down && (key == K_PGUP || key == K_MWHEELUP)) {
        std::size_t n = _control ? 10 : 1;
        if (_scroll_offset + n < _buffer.num_rows()) {
            _scroll_offset += n;
        } else {
            _scroll_offset = _buffer.num_rows();
        }
        return true;
    } else if (down && (key == K_PGDN || key == K_MWHEELDOWN)) {
        std::size_t n = _control ? 10 : 1;
        if (_scroll_offset > n) {
            _scroll_offset -= n;
        } else {
            _scroll_offset = 0;
        }
        return true;
    } else {
        return _input.key_event(key, down);
    }
}

//------------------------------------------------------------------------------
void console::execute(string::view text)
{
    parser::text args(text);
    if (!args.tokens().size()) {
        return;
    }

    // check for console commands
    auto command = _commands.find(args.tokens()[0]);
    if (command != _commands.cend()) {
        return command->second->execute(args);
    }

    // check for config variables
    config::system* config = config::system::singleton();
    config::variable_base* variable = config ? config->find(args.tokens()[0]) : nullptr;
    if (variable) {
        return command_set(args);
    }

    log::message("unknown variable or command: '^fff%s^xxx'\n", args.tokens()[0].c_str());
}

//------------------------------------------------------------------------------
void console::command_set(parser::text const& args)
{
    assert(args.tokens().size());
    assert(config::system::singleton());
    std::size_t first = stricmp(args.tokens()[0], "set") ? 0 : 1;
    std::size_t nargs = args.tokens().size() - first;

    if (!nargs || nargs > 2) {
        log::message("usage: set <^fffvariable^xxx> [^fffvalue^xxx]\n");
        return;
    }

    config::system* config = config::system::singleton();
    config::variable_base* variable = config->find(args.tokens()[first]);
    if (!variable) {
        log::message("unknown variable: '^fff%s^xxx'\n", args.tokens()[first].c_str());
        return;
    } else if (nargs == 1) {
        log::message("^fff%s^xxx = '^fff%s^xxx'\n", variable->name().c_str(), variable->value().c_str());
    } else if (nargs == 2) {
        variable->set(args.tokens()[first + 1]);
    }
}

//------------------------------------------------------------------------------
void console::command_list(parser::text const& /*args*/)
{
    for (auto it : _singleton->_commands) {
        log::message("%s\n", it.first.c_str());
    }
}
