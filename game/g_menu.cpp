// g_menu.cpp
//

#include "precompiled.h"
#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////
namespace menu {

//------------------------------------------------------------------------------
void window::init ()
{
    // base

    _submenus.emplace_back(new menu::window);  // local
    _submenus.emplace_back(new menu::window);  // network
    _submenus.emplace_back(new menu::window);  // options

    add_button<conditional_button>("Resume", vec2i(64,32), vec2i(96,32), &g_Game->svs.active, [](){
        g_Game->resume();
    });

    add_button<submenu_button>("Network Game", vec2i(192,32), vec2i(96,32), this, _submenus[1].get());
    add_button<submenu_button>("Local Game", vec2i(320,32), vec2i(96,32), this, _submenus[0].get());
    add_button<submenu_button>("Game Options", vec2i(448,32), vec2i(96,32), this, _submenus[2].get());

    add_button<button>("Quit", vec2i(576,32), vec2i(96,32), [](){
        g_Game->stop_server();
        g_Game->stop_client();
        application::singleton()->quit(0);
    });

    // local

    _submenus[0]->add_button<button>("New Round", vec2i(48,80), vec2i(64,32), [](){
        g_Game->stop_server();
        g_Game->stop_client();
        g_Game->start_server_local();
        g_Game->start_client_local();
    });

    _submenus[0]->add_button<button>("Reset", vec2i(48,128), vec2i(64,32), [](){
        g_Game->reset();
    });

    // network

    _submenus[1]->_submenus.emplace_back(new menu::window);
    _submenus[1]->_submenus.emplace_back(new menu::window);
    _submenus[1]->add_button<submenu_button>("Host", vec2i(48,80), vec2i(64,24), _submenus[1].get(), _submenus[1]->_submenus[0].get());
    _submenus[1]->add_button<submenu_button>("Join", vec2i(128,80), vec2i(64,24), _submenus[1].get(), _submenus[1]->_submenus[1].get());

    // network->host

    _submenus[1]->_submenus[0]->add_button<host_button>(vec2i(144,128), vec2i(256,24), [](){
        g_Game->start_server();
        if (!g_Game->_dedicated) {
            g_Game->start_client_local();
        }
    });

    // network->join

    _submenus[1]->_submenus[1]->add_button<button>("Refresh", vec2i(240,80), vec2i(64,24), [](){
        g_Game->info_ask( );
    });

    for (int ii = 0; ii < 8; ++ii) {
        _submenus[1]->_submenus[1]->add_button<server_button>(vec2i(144,128+ii*32), vec2i(256,24), g_Game->cls.servers[ii].name, &g_Game->cls.servers[ii].ping, [ii](){
            g_Game->connect_to_server(ii);
        });
    }

    // options

    _submenus[2]->add_button<client_button>("Player", vec2i(64,104), vec2i(96,80));
}

//------------------------------------------------------------------------------
void window::shutdown()
{
    _buttons.clear();
    _submenus.clear();
}

//------------------------------------------------------------------------------
bool window::activate(submenu_button* button, menu::window* submenu)
{
    if (submenu == _active_menu)
    {
        _active_button = nullptr;
        _active_menu = nullptr;
        return false;   // deactivates caller
    }

    if (_active_button) {
        _active_button->deactivate();
    }
    _active_button = button;
    _active_menu = submenu;

    return true;
}

//------------------------------------------------------------------------------
void window::draw(render::system* renderer) const
{
    // draw buttons
    for (auto const& button : _buttons) {
        button->draw(renderer);
    }

    // draw submenu
    if (_active_menu) {
        _active_menu->draw(renderer);
    }
}

//------------------------------------------------------------------------------
bool window::key_event(int key, bool down)
{
    if (_active_menu && _active_menu->key_event(key, down)) {
        return true;
    }

    for (auto& button : _buttons) {
        if (button->key_event(key, down)) {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
bool window::cursor_event(vec2i position)
{
    if (_active_menu && _active_menu->cursor_event(position)) {
        return true;
    }

    for (auto& button : _buttons) {
        if (button->cursor_event(position)) {
            return true;
        }
    }

    return false;
}

} // namespace menu
