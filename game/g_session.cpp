// g_session.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_keys.h"
#include "cm_parser.h"
#include "g_aicontroller.h"
#include "g_player.h"
#include "resource.h"
#include "version.h"

#include <cstdarg>
#include <numeric>

// global object
game::session* g_Game;

void find_server(bool connect);

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
session::session()
    : _menu_active(true)
    , _dedicated(false)
    , _upgrade_frac("g_upgradeFrac", 0.5f, config::archive|config::server, "upgrade fraction")
    , _upgrade_penalty("g_upgradePenalty", 0.2f, config::archive|config::server, "upgrade penalty")
    , _upgrade_min("g_upgradeMin", 0.2f, config::archive|config::server, "minimum upgrade fraction")
    , _upgrades("g_upgrades", true, config::archive|config::server, "enable upgrades")
    , _net_master("net_master", "oedhead.no-ip.org", config::archive, "master server hostname")
    , _net_server_name("net_serverName", "Quark Server", config::archive, "local server name")
    , _cl_name("ui_name", "", config::archive, "user info: name")
    , _cl_color("ui_color", "255 0 0", config::archive, "user info: color")
    , _cl_weapon("ui_weapon", 0, config::archive, "user info: weapon")
    , _timescale("timescale", 1.f, config::server, "")
    , _restart_time(time_value::zero)
    , _zoom(1)
    , _worldtime(time_value::zero)
    , _frametime(time_value::zero)
    , _framenum(0)
    , _cursor(0,0)
    , _show_cursor(true)
    , _num_messages(0)
    , _net_graph("net_graph", false, config::archive, "draw network usage graph")
    , _client_button_down(false)
    , _server_button_down(false)
    , _client_say(false)
    , _command_quit("quit", &session::command_quit)
    , _command_disconnect("disconnect", this, &session::command_disconnect)
    , _command_connect("connect", this, &session::command_connect)
{
    log::set(this);
    g_Game = this;
}

//------------------------------------------------------------------------------
result session::init (string::view cmdline)
{
    _renderer = application::singleton()->window()->renderer();
    {
        float num = _renderer->view().size.x * ((640.f - 8.f) / 640.f);
        float den = _renderer->monospace_size(" ").x;
        _console.resize(static_cast<std::size_t>(num / den));
    }

    uint8_t utf8[] = {
        0xce, 0xbc, /* mu */
        0xd9, 0x83, /* kaf */
        0xef, 0xbb, 0x99, /* kaf isolated form */
        '\r', '\n', '\0'
    };
    _console.printf("%s", utf8);

    for ( int i=0 ; i<MAX_MESSAGES ; i++ )
        memset( _messages[i].string, 0, MAX_STRING );

    for ( int i=0 ; i<MAX_SERVERS ; i++ )
    {
        cls.servers[i].active = false;
        memset( cls.servers[i].name, 0, 32 );
    }

    svs.active = false;
    svs.local = false;

    for (auto& cl : svs.clients) {
        cl.active = false;
        cl.local = false;
        cl.info.name.fill('\0');
        cl.info.color = color3(1,1,1);
    }

    memset( _clientsay, 0, LONG_STRING );

    strcpy( svs.name, _net_server_name );

    cls.active = false;
    cls.local = false;
    cls.number = -1;

    _clients[0].input.bind({
        {K_ALT, usercmd::modifier::alternate},
        {K_CTRL, usercmd::modifier::control},
        {K_SHIFT, usercmd::modifier::shift},
        {K_MOUSE1, usercmd::button::select},
        {K_MOUSE2, usercmd::action::command},
        {'m', usercmd::action::move},
        {'1', usercmd::action::weapon_1},
        {'2', usercmd::action::weapon_2},
        {'3', usercmd::action::weapon_3},
        {'s', usercmd::action::toggle_shield},
        {'=', usercmd::action::zoom_in},
        {'-', usercmd::action::zoom_out},
        {K_MWHEELUP, usercmd::action::zoom_in},
        {K_MWHEELDOWN, usercmd::action::zoom_out},
    });

    init_client();

    _menu.init( );
    _world.init( );

    if (cmdline.contains("dedicated")) {
        _dedicated = true;
        start_server( );
    }

    // sound indices are shared over the network so sounds
    // need to be registed in the same order on all clients
    pSound->load_sound("assets/sound/tank_move.wav");
    pSound->load_sound("assets/sound/tank_idle.wav");
    pSound->load_sound("assets/sound/tank_explode.wav");
    pSound->load_sound("assets/sound/turret_move.wav");
    pSound->load_sound("assets/sound/blaster_fire.wav");
    pSound->load_sound("assets/sound/blaster_impact.wav");
    pSound->load_sound("assets/sound/cannon_fire.wav");
    pSound->load_sound("assets/sound/cannon_impact.wav");
    pSound->load_sound("assets/sound/missile_flight.wav");

    return result::success;
}

//------------------------------------------------------------------------------
void session::shutdown()
{
    stop_client( );
    shutdown_client();

    _world.shutdown( );
    _menu.shutdown( );
}

//------------------------------------------------------------------------------
result session::run_frame(time_delta time)
{
    get_packets( );

    _frametime += time;

    // step session

    if (_frametime > time_value(_framenum * FRAMETIME)) {
        _framenum++;
        _net_bytes[_framenum % _net_bytes.size()] = 0;
    }

    // step world

    if (!_menu_active || svs.active) {
        // clamp world step size
        _worldtime += std::min(time, FRAMETIME) * _timescale;

        // update client
        if (!_dedicated) {
            if (_player && _player->is_type<player>()) {
                static_cast<player*>(const_cast<object*>(_player.get()))->update_usercmd(_clients[0].input.generate(), _worldtime);
            }
        }

        if (_worldtime > time_value((1 + _world.framenum()) * FRAMETIME) && svs.active) {
            _world.run_frame();
            if (!svs.local) {
                write_frame();
            }
        }
    }

    send_packets( );

    // draw everything

    update_screen();

    // update sound

    pSound->update( );

    if (_restart_time != time_value::zero && (_frametime > _restart_time) && !_menu_active ) {
        restart();
    }

    return result::success;
}

//------------------------------------------------------------------------------
void session::update_screen()
{
    _renderer->begin_frame();

    draw_world();

    //
    // calculate view for ui
    //

    render::view view = {};

    view.raster = true;
    view.size = vec2(640, 480);
    view.origin = view.size * 0.5f;
    _renderer->set_view(view);

    //
    // draw ui
    //

    draw_menu();

    draw_messages();

    draw_console();

    draw_netgraph();

    _renderer->end_frame();
}

//------------------------------------------------------------------------------
void session::draw_menu()
{
    // draw menu

    if (_menu_active) {
        _menu.draw(_renderer);
    }
}

//------------------------------------------------------------------------------
void session::char_event(int key)
{
    if (_console.char_event(key) || _console.active()) {
        return;
    }

    if (_client_button_down) {
        if (_menu_active) {
            std::size_t len = strnlen(cls.info.name.data(), cls.info.name.size());

            if (key == K_BACKSPACE && len) {
                cls.info.name[len - 1] = 0;
            } else if (key == K_ENTER) {
                _client_button_down = false;
            } else if (key <= K_SPACE) {
                return;
            } else if (key >= K_BACKSPACE) {
                return;
            } else if (len < 13) {
                cls.info.name[len + 1] = 0;
                cls.info.name[len] = narrow_cast<char>(key);
            }
            return;
        } else {
            _client_button_down = false;
        }
    } else if (_server_button_down) {
        if (_menu_active) {
            std::size_t len = strnlen(svs.name, countof(svs.name));

            if (key == K_BACKSPACE && len) {
                svs.name[strlen(svs.name) - 1] = 0;
            } else if (key == K_ENTER) {
                _server_button_down = false;
            } else if (key < K_SPACE) {
                return;
            } else if (key >= K_BACKSPACE) {
                return;
            } else if (len < countof(svs.name)) {
                svs.name[len + 1] = 0;
                svs.name[len] = narrow_cast<char>(key);
            }

            return;
        } else {
            _server_button_down = false;
        }
    } else if (_client_say) {
        if (key == K_BACKSPACE) {
            _clientsay[strlen(_clientsay) - 1] = 0;
        } else if (key == K_ENTER && strlen(_clientsay)) {
            if (!_clientsay[0]) {
                return;
            }

            if (svs.active || cls.active) {
                // say it
                _netchan.write_byte(clc_say);
                _netchan.write_string(_clientsay);

                if (svs.active && !svs.local) {
                    if (_dedicated) {
                        write_message(va("[Server]: %s", _clientsay));
                    } else {
                        write_message(va("^%x%x%x%s^xxx: %s",
                            (int)(cls.info.color.r * 15.5f),
                            (int)(cls.info.color.g * 15.5f),
                            (int)(cls.info.color.b * 15.5f),
                            cls.info.name.data(), _clientsay));
                    }
                }
            }

            memset(_clientsay, 0, LONG_STRING);
            _client_say = false;
        } else if (key == K_ENTER) {
            _client_say = false;
        } else if (key == K_ESCAPE) {
            memset(_clientsay, 0, LONG_STRING);
            _client_say = false;
        } else if (key < K_SPACE) {
            return;
        } else if (key > K_BACKSPACE) {
            return;
        } else if (strlen(_clientsay) < LONG_STRING) {
            _clientsay[strlen(_clientsay) + 1] = 0;
            _clientsay[strlen(_clientsay)] = narrow_cast<char>(key);
        }

        return;
    } else if (key == K_ENTER) {
        _client_say = true;
        return;
    } else if (key == '/') {
        _client_say = true;
        _clientsay[0] = '/';
        return;
    }
}

//------------------------------------------------------------------------------
void session::key_event(int key, bool down)
{
    if (_console.key_event(key, down) || _console.active()) {
        return;
    }

    if (_menu_active) {
        if (_menu.key_event(key, down)) {
            return;
        }
    }

    if (key == K_PGDN && down) {
        for (int i = 0; i < MAX_MESSAGES; i++) {
            _messages[i].time = _frametime;
        }
    } else if ((key == K_MWHEELUP || key == '+' || key =='=') && down) {
        _zoom = _zoom * 1.1f;
    } else if ((key == K_MWHEELDOWN || key == '-') && down) {
        _zoom = _zoom / 1.1f;
    }

    // user commands here

    if (!_dedicated) {
        if (_player && _player->is_type<player>()) {
            if (_clients[0].input.key_event(key, down)) {
                return;
            }
        }
        for (int ii = 0; ii < MAX_PLAYERS; ++ii) {
            //game::tank* player = _world.player(ii);
            //if (player && _clients[ii].input.key_event(key, down)) {
            //    player->update_usercmd(_clients[ii].input.generate());
            //}
        }
    }

    if (!down) {
        return;
    }

    if (key >= K_F1 && key <= K_F12) {
        cls.number = key - K_F1;
        std::vector<game::object const*> controllers;
        for (auto const& obj : _world.objects()) {
            if (obj->is_type<aicontroller>() || obj->is_type<player>()) {
                controllers.push_back(obj);
            }
        }

        if (cls.number >= 0 && cls.number < controllers.size()) {
            _player = controllers[cls.number];
        } else {
            _player = nullptr;
        }
    }

    // menu commands

    if (key == K_ESCAPE) {
        _menu_active ^= 1;
        return;
    }

    if (key == K_F2) {
        byte    msg[2];

        msg[0] = svc_restart;
        msg[1] = 5;

        broadcast(2, msg);

        _restart_time = _frametime + RESTART_TIME;
        return;
    }
}

vec2 g_cursor;

//------------------------------------------------------------------------------
void session::cursor_event(vec2 position)
{
    vec2i size = _renderer->window()->size();
    _cursor.x = static_cast<int>(position.x * 640 / size.x);
    _cursor.y = static_cast<int>(position.y * 480 / size.y);

    _clients[0].input.cursor_event(position / vec2(size) * vec2(1,-1) + vec2(0,1));
    if (_player && _player->is_type<player>()) {
        static_cast<player*>(const_cast<object*>(_player.get()))->update_usercmd(_clients[0].input.generate_direct(), _worldtime);
    }

    if (_menu_active) {
        _menu.cursor_event(_cursor);
    }
    g_cursor = position;
}

//------------------------------------------------------------------------------
void session::gamepad_event(int /*index*/, gamepad const& /*pad*/)
{
    if (!_dedicated) {
        //game::tank* player = _world.player(index);
        //if (player) {
        //    _clients[index].input.gamepad_event(pad);
        //    player->update_usercmd(_clients[index].input.generate());
        //}
    }
}

//------------------------------------------------------------------------------
void session::draw_netgraph()
{
    constexpr float width = 580.0f;
    constexpr float height = 48.0f;
    constexpr float base = 128.0f; // 10 kbps

    if (!_net_graph) {
        return;
    }

    float sum = 0;
    float max = 0;

    for (size_t ii = 1; ii < _net_bytes.size() && ii < _framenum; ++ii) {
        std::size_t value = _net_bytes[(_framenum - ii) % _net_bytes.size()];
        sum += value;
        max = std::max(max, static_cast<float>(value));
    }

    float avg = sum / (std::min<std::size_t>(_framenum, _net_bytes.size()) - 1);
    float scale = std::min<float>(height / base, height / max);

    //
    // draw graph lines
    //

    for (size_t ii = 1; ii < _net_bytes.size() && ii < _framenum; ++ii) {
        float x0 = width - (ii - 1) * width / (_net_bytes.size() - 2);
        float x1 = width - (ii - 0) * width / (_net_bytes.size() - 2);
        float y0 = 480.0f - _net_bytes[(_framenum - ii + 0) % _net_bytes.size()] * scale;
        float y1 = 480.0f - _net_bytes[(_framenum - ii - 1) % _net_bytes.size()] * scale;

        _renderer->draw_line(vec2(x0,y0), vec2(x1, y1), color4(1,1,1,1), color4(1,1,1,1));
    }

    //
    // draw guide lines
    //

    {
        float ymax = std::min<float>(478.0f, 480.0f - max * scale);
        float yavg = std::min<float>(478.0f, 480.0f - avg * scale);
        float alpha_avg = std::min<float>(1, std::abs(ymax - yavg) * 0.1f);

        _renderer->draw_line(vec2(width, 480.0f), vec2(width, ymax - 8.0f), color4(1,1,1,1), color4(1,1,1,1));
        _renderer->draw_line(vec2(0, ymax), vec2(width, ymax), color4(1,1,1,1), color4(1,1,1,1));
        _renderer->draw_line(vec2(0, yavg), vec2(width, yavg), color4(0.5f,1,0.75f,alpha_avg), color4(0.5f,1,0.7f,alpha_avg));

        string::buffer smax(va("%0.1f kbps", CHAR_BIT * max / (FRAMETIME.to_seconds() * 1024.0f)));
        string::buffer savg(va("%0.1f kbps", CHAR_BIT * avg / (FRAMETIME.to_seconds() * 1024.0f)));

        _renderer->draw_string(smax, vec2(638.0f - _renderer->string_size(smax).x, ymax), color4(1,1,1,1));
        _renderer->draw_string(savg, vec2(638.0f - _renderer->string_size(savg).x, yavg), color4(1,1,1,alpha_avg));
    }
}

//------------------------------------------------------------------------------
void session::reset()
{
    _world.reset( );
    _worldtime = time_value::zero;
}

//------------------------------------------------------------------------------
void session::resume()
{
    _menu_active = false;
}

//------------------------------------------------------------------------------
void session::new_game()
{
    _restart_time = time_value::zero;
    _world.clear_particles( );

    if (!svs.active) {
        return;
    }

    //
    //  reset world
    //

    _world.reset( );
    _worldtime = time_value::zero;

    //
    //  reset players
    //

    for ( int i=0 ; i<MAX_PLAYERS ; i++ )
    {
        if (svs.local && i > 1 )
            break;
        else if (svs.active && !svs.clients[i].active )
            continue;

        if (_restart_time == time_value::zero/* || !_world.player(i)*/) {
            spawn_player(i);
        }
    }

    _menu_active = false;
}

//------------------------------------------------------------------------------
void session::restart()
{
    _restart_time = time_value::zero;

    if (!svs.active) {
        return;
    }

    for (int ii = 0; ii < MAX_PLAYERS; ++ii) {
        if (svs.local && ii > 1) {
            break;
        } else if (svs.local && !svs.clients[ii].active) {
            continue;
        }

        //assert(_world.player(ii) != nullptr);
        //_world.player(ii)->respawn();
    }

    _menu_active = false;
}

//------------------------------------------------------------------------------
void session::spawn_player(std::size_t /*num*/)
{
    //
    //  initialize tank object
    //

    //assert(_world.player(num) == nullptr);
    //game::tank* player = _world.spawn_player(num);
    //player->_color = color4(svs.clients[num].info.color);
    //player->_weapon = svs.clients[num].info.weapon;
    //player->_client = _clients + num;

    //player->respawn();
}

//------------------------------------------------------------------------------
result session::message(char const* format, ...)
{
    auto& msg = _messages[_num_messages++ % MAX_MESSAGES];
    va_list list;

    va_start(list, format);
    vsnprintf(msg.string, countof(msg.string), format, list);
    va_end(list);

    msg.time = _frametime;
    return result::success;
}

//------------------------------------------------------------------------------
void session::write_message(string::view message, bool broadcast)
{
    auto& msg = _messages[_num_messages++ % MAX_MESSAGES];

    if (svs.active && broadcast) {
        broadcast_print(message);
    }

    strcpy(msg.string, message);
    msg.time = _frametime;
}

//------------------------------------------------------------------------------
void session::draw_messages()
{
    constexpr time_delta view_time = time_delta::from_seconds(15);
    constexpr time_delta fade_time = time_delta::from_seconds(3);

    float ypos = _renderer->view().size.y - 36.f;

    for (int ii = 0; ii < _num_messages && ii < MAX_MESSAGES; ++ii) {
        message_t const& msg = _messages[(_num_messages - ii - 1) % MAX_MESSAGES];

        if (msg.time + view_time > _frametime) {
            float alpha = (msg.time + (view_time - fade_time) > _frametime ? 1.0f : (msg.time + view_time - _frametime) / fade_time);

            _renderer->draw_string(msg.string, vec2(8.f,ypos), color4(1,1,1,alpha));

            ypos -= 12.f;
        }
    }

    int width = static_cast<int>(_renderer->view().size.x);
    int height = static_cast<int>(_renderer->view().size.y);

    // draw console/chat box

    if (_client_say) {
        _renderer->draw_string("say:", vec2(vec2i(width/4,height-16)), menu::colors[7]);
        _renderer->draw_string(_clientsay, vec2(vec2i(width/4+32,height-16)), menu::colors[7]);
    }
}

//------------------------------------------------------------------------------
void session::draw_console()
{
    if (!_console.height()) {
        return;
    }

    int width = static_cast<int>(_renderer->view().size.x);
    int height = static_cast<int>(_renderer->view().size.y);

    int yoffset = int(height * _console.height());
    _renderer->draw_box(
        vec2(vec2i(width, yoffset)),
        vec2(vec2i(width, yoffset) / 2),
        color4(.1f,.1f,.1f,.8f));

    int ystep = int(_renderer->monospace_size(" ").y);

    // draw build string
    {
        string::literal build(BUILD_STRING);
        vec2 size = _renderer->monospace_size(build);
        _renderer->draw_monospace(build, vec2(vec2i(width - int(size.x) - 4, yoffset - 8)), menu::colors[1]);
    }

    // draw input text
    {
        char buf[260] = "]";
        console_input const& input = _console.input();
        strzcpy(buf + 1, string::view(input.begin(), input.end()), countof(buf) - 1);
        _renderer->draw_monospace(buf, vec2(vec2i(4, yoffset - 8)), menu::colors[6]);
        // draw input cursor
        if ((int)(_frametime.to_seconds() * 2.5f) % 2 == 0) {
            buf[input.cursor() - input.begin() + 1] = '_';
            buf[input.cursor() - input.begin() + 2] = '\0';
            _renderer->draw_monospace(buf, vec2(vec2i(4, yoffset - 8)), menu::colors[6]);
        }
    }

    // draw console rows
    std::size_t num_rows = _console.num_rows();
    for (std::size_t ii = 0; ii + _console.scroll() < num_rows; ++ii) {
        int y = yoffset - narrow_cast<int>(ii + 1) * ystep - 8;
        if (y < 0) {
            break;
        }
        _renderer->draw_monospace(string::view(_console.get_row(ii + _console.scroll())),
                                  vec2(vec2i(4, y)),
                                  menu::colors[6]);
    }
}

//------------------------------------------------------------------------------
void session::command_quit(parser::text const&)
{
    application::singleton()->quit(0);
}

//------------------------------------------------------------------------------
void session::command_disconnect(parser::text const&)
{
    stop_client();
    stop_server();
}

//------------------------------------------------------------------------------
void session::command_connect(parser::text const& args)
{
    if (args.tokens().size() > 2) {
    } else if (args.tokens().size() == 2) {
        connect_to_server(args.tokens()[1]);
    } else {
        connect_to_server(-1);
    }
}

//------------------------------------------------------------------------------
void session::print(log::level level, char const* msg)
{
    switch (level) {
        case log::level::message:
            _console.printf("%s", msg);
            break;

        case log::level::warning:
            _console.printf("^ff0warning^xxx: %s", msg);
            break;

        case log::level::error:
            _console.printf("^f00error^xxx: %s", msg);
            break;
    }
}

} // namespace game
