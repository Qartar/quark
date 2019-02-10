// g_usercmd.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_usercmd.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
void usercmdgen::reset(bool unbind_all/* = false*/)
{
    _button_state = button::none;
    _gamepad_state = {};
    _modifier_state = modifier::none;
    _queue_begin = 0;
    _queue_end = 0;
    if (unbind_all) {
        _bindings.clear();
    }
}

//------------------------------------------------------------------------------
void usercmdgen::bind(int key, usercmdgen::binding button)
{
    _bindings[key] = button;
}

//------------------------------------------------------------------------------
void usercmdgen::unbind(int key)
{
    _bindings.erase(key);
}

//------------------------------------------------------------------------------
bool usercmdgen::key_event(int key, bool down)
{
    auto it = _bindings.find(key);
    if (it != _bindings.end()) {
        if (std::holds_alternative<action>(it->second) && down) {
            if (_queue_end - _queue_begin < _queue_size) {
                usercmd& cmd = _queue[_queue_end++ % _queue_size];
                cmd = generate_direct();
                cmd.action = std::get<action>(it->second);
            }
        } else if (std::holds_alternative<button>(it->second)) {
            if (down) {
                _button_state |= std::get<button>(it->second);
            } else {
                _button_state &= ~std::get<button>(it->second);
            }
        } else if (std::holds_alternative<modifier>(it->second)) {
            if (down) {
                _modifier_state |= std::get<modifier>(it->second);
            } else {
                _modifier_state &= ~std::get<modifier>(it->second);
            }
        }
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
void usercmdgen::cursor_event(vec2 position)
{
    _cursor_state = position;
}

//------------------------------------------------------------------------------
void usercmdgen::gamepad_event(gamepad const& pad)
{
    _gamepad_state = pad;
}

//------------------------------------------------------------------------------
int usercmdgen::state(usercmdgen::button button) const
{
    return !!(_button_state & button);
}

//------------------------------------------------------------------------------
usercmd usercmdgen::generate()
{
    if (_queue_begin < _queue_end) {
        return _queue[_queue_begin++ % _queue_size];
    } else {
        return generate_direct();
    }
}

//------------------------------------------------------------------------------
usercmd usercmdgen::generate_direct() const
{
    usercmd cmd{};
    cmd.cursor = _cursor_state;
    cmd.action = usercmd::action::none;
    cmd.buttons = _button_state;
    cmd.modifiers = _modifier_state;
    return cmd;
}

} // namespace game
