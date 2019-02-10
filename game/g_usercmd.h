// g_usercmd.h
//

#pragma once

#include "cm_enumflag.h"
#include "cm_vector.h"

#include <cstdint>
#include <map>
#include <variant>

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
struct gamepad
{
    enum side
    {
        left = 0,
        right = 1,
    };

    //! left and right thumbstick state, normalized to unit length
    vec2 thumbstick[2];
    //! left and right trigger state, normalized to [0,1]
    float trigger[2];
};

//------------------------------------------------------------------------------
struct usercmd
{
    enum class action
    {
        none,
        select,
        move,
        weapon_1,
        weapon_2,
        weapon_3,
        zoom_in,
        zoom_out,
    };

    enum class button
    {
        none        = 0,
        select      = (1 << 0),
        zoom_in     = (1 << 1),
        zoom_out    = (1 << 2),
    };
    ENUM_FLAG_FRIEND_OPERATORS(usercmd::button);

    enum class modifier
    {
        none        = 0,
        alternate   = (1 << 0),
        control     = (1 << 1),
        shift       = (1 << 2),
    };
    ENUM_FLAG_FRIEND_OPERATORS(usercmd::modifier);

    vec2 cursor;
    action action;
    button buttons;
    modifier modifiers;
};

//------------------------------------------------------------------------------
class usercmdgen
{
public:
    using action = decltype(usercmd::action);
    using button = usercmd::button;
    using modifier = usercmd::modifier;
    using binding = std::variant<action, button, modifier>;

    usercmdgen()
        : _button_state(button::none)
        , _modifier_state(modifier::none)
        , _gamepad_state{}
        , _queue_begin(0)
        , _queue_end(0)
    {}

    void reset(bool unbind_all = false);
    void bind(int key, binding button);
    void unbind(int key);
    template<std::size_t Size> void bind(std::pair<int, binding> const (&bindings)[Size]);

    bool key_event(int key, bool down);
    void cursor_event(vec2 position);
    void gamepad_event(gamepad const& pad);

    int state(button button) const;
    usercmd generate();
    usercmd generate_direct() const;

protected:
    std::map<int, binding> _bindings;
    button _button_state;
    modifier _modifier_state;
    vec2 _cursor_state;
    gamepad _gamepad_state;

    static constexpr std::size_t _queue_size = 64;
    usercmd _queue[_queue_size];
    std::size_t _queue_begin;
    std::size_t _queue_end;
};

//------------------------------------------------------------------------------
template<size_t Size> void usercmdgen::bind(std::pair<int, binding> const (&bindings)[Size])
{
    for (std::size_t ii = 0; ii < Size; ++ii) {
        bind(bindings[ii].first, bindings[ii].second);
    }
}

} // namespace game
