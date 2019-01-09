// g_menu.h
//

#pragma once

#include "cm_string.h"
#include "g_projectile.h"
#include "g_weapon.h"

#include <functional>
#include <memory>
#include <vector>

namespace render {
class system;
} // namespace render

////////////////////////////////////////////////////////////////////////////////
namespace menu {

class window;

//------------------------------------------------------------------------------
constexpr color4 colors[] =
{
    {0.000,0.000,0.000,1},
    {0.125,0.125,0.125,1},
    {0.250,0.250,0.250,1},
    {0.375,0.375,0.375,1},
    {0.500,0.500,0.500,1},
    {0.625,0.625,0.625,1},
    {0.750,0.750,0.750,1},
    {0.875,0.875,0.875,1},
    {1.000,1.000,1.000,1},
};

//------------------------------------------------------------------------------
class button
{
public:
    button(string::view text, vec2i center, vec2i size)
        : _text(text)
        , _rectangle(rect::from_center(center, size))
        , _down(false)
        , _over(false)
    {}

    template<typename Tfunc>
    button(string::view text, vec2i center, vec2i size, Tfunc&& func)
        : button(text, center, size)
    {
        _func = std::move(func);
    }

    virtual ~button() {}

    virtual bool key_event(int key, bool down);
    virtual bool cursor_event(vec2i position);

    virtual void draw(render::system* renderer) const;

protected:
    string::buffer _text;
    rect _rectangle;
    std::function<void()> _func;

    bool _down;
    bool _over;

protected:
    enum text_flags {
        align_default   = 0,

        // horizontal alignment
        halign_center   = align_default,
        halign_left     = 1 << 0,
        halign_right    = 1 << 1,

        // vertical alignment
        valign_center   = align_default,
        valign_top      = 1 << 2,
        valign_bottom   = 1 << 3,
    };

    void draw_rectangle(render::system* renderer, rect const& rect, color4 color) const;
    void draw_rectangle(render::system* renderer, rect const& rect, color4 color, color4 border_color) const;
    void draw_text(render::system* renderer, rect const& rect, string::view text, color4 color, int flags = align_default, float margin = 4.0f) const;
};

//------------------------------------------------------------------------------
class server_button : public button
{
public:
    server_button(vec2i position, vec2i size, char const* name_ptr, time_delta const* ping_ptr, std::function<void()>&& op_click);

    virtual bool key_event(int key, bool down) override;

    virtual void draw(render::system* renderer) const override;

protected:
    char const* _name_ptr;
    time_delta const* _ping_ptr;

    rect _join_rectangle;
    rect _text_rectangle;
};

//------------------------------------------------------------------------------
class host_button : public button
{
public:
    host_button(vec2i position, vec2i size, std::function<void()>&& op_click);

    virtual bool key_event(int key, bool down) override;
    virtual bool cursor_event(vec2i position) override;

    virtual void draw(render::system* renderer) const override;

protected:
    rect _create_rectangle;
    rect _text_rectangle;

    bool _create_over;
    bool _text_over;

    bool _create_down;
    bool _text_down;
};

//------------------------------------------------------------------------------
class conditional_button : public button
{
public:
    conditional_button(string::view text, vec2i position, vec2i size, bool *condition_ptr, std::function<void()>&& op_click);

    virtual bool key_event(int key, bool down) override;

    virtual void draw(render::system* renderer) const override;

protected:
    bool* _condition_ptr;
};

//------------------------------------------------------------------------------
class weapon_button : public button
{
public:
    weapon_button(game::weapon_type type, vec2i position, vec2i size);

    virtual void draw(render::system* renderer) const override;

protected:
    game::weapon_type _type;

    static const string::literal _strings[];
};

//------------------------------------------------------------------------------
class client_button : public button
{
public:
    client_button(string::view text, vec2i position, vec2i size, color3* color_ptr);

    virtual bool key_event(int key, bool down) override;
    virtual bool cursor_event(vec2i position) override;

    virtual void draw(render::system* renderer) const override;

protected:
    color3* _color_ptr;
    int _color_index;

    bool _text_down;
    bool _text_over;

    rect _text_rectangle;

    std::array<weapon_button, 3> _weapons;
};

//------------------------------------------------------------------------------
class submenu_button : public button
{
public:
    submenu_button (string::view text, vec2i position, vec2i size, menu::window *pParent, menu::window *pMenu);

    virtual void draw(render::system* renderer) const override;

    void deactivate() { _active = false; }

protected:
    menu::window* _parent;
    menu::window* _menu;
    bool _active;
};

//------------------------------------------------------------------------------
class window
{
public:
    window ()
        : _active_button(nullptr)
        , _active_menu(nullptr)
    {}

    void init ();
    void shutdown ();

    template<typename T, typename... Args>
    void add_button(Args&& ...args);

    bool key_event(int key, bool down);
    bool cursor_event(vec2i position);

    void draw(render::system* renderer) const;

    bool activate(submenu_button* button, menu::window* submenu);

protected:
    std::vector<std::unique_ptr<menu::button>> _buttons;
    std::vector<std::unique_ptr<menu::window>> _submenus;

    submenu_button* _active_button;
    menu::window* _active_menu;
};

template<typename T, typename... Args>
void window::add_button(Args&& ...args)
{
    _buttons.push_back(std::make_unique<T>(args...));
}

} // namespace menu
