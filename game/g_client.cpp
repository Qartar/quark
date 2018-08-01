// g_client.cpp
//

#include "precompiled.h"
#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////
namespace game {

const float colorMinFrac = 0.75f;

//------------------------------------------------------------------------------
void session::init_client()
{
    float csum;

    _client_button_down = 0;
    _client_say = 0;

    if (strlen(_cl_name)) {
        strncpy(cls.info.name.data(), _cl_name, cls.info.name.size());
#if defined(_WIN32)
    } else {
        DWORD length = narrow_cast<DWORD>(cls.info.name.size());
        GetUserNameA(cls.info.name.data(), (LPDWORD )&length);
#endif
    }

    {
        int r = 255, g = 255, b = 255;
        sscanf(_cl_color, "%d %d %d", &r, &g, &b);
        cls.info.color.r = static_cast<float>(r) * (1.f / 255.f);
        cls.info.color.g = static_cast<float>(g) * (1.f / 255.f);
        cls.info.color.b = static_cast<float>(b) * (1.f / 255.f);
    }

    csum = cls.info.color.r + cls.info.color.g + cls.info.color.b;
    if ( csum < colorMinFrac ) {
        if ( csum == 0.0f ) {
            cls.info.color.r = cls.info.color.g = cls.info.color.b = colorMinFrac * 0.334f;
        } else {
            float   invsum = colorMinFrac / csum;

            cls.info.color.r *= invsum;
            cls.info.color.g *= invsum;
            cls.info.color.b *= invsum;
        }
    }

    cls.info.weapon = static_cast<weapon_type>((int)_cl_weapon);

    _net_bytes.fill(0);
}

//------------------------------------------------------------------------------
void session::shutdown_client()
{
    _cl_name = string::view(cls.info.name.data());
    _cl_color = va("%i %i %i", (int )(cls.info.color.r*255), (int )(cls.info.color.g*255), (int )(cls.info.color.b*255) );
    _cl_weapon = static_cast<int>(cls.info.weapon);
}

//------------------------------------------------------------------------------
void session::stop_client ()
{
    if (cls.active) {
        _netchan.write_byte(clc_disconnect);
        _netchan.transmit();
        _netchan.reset();
    }

    cls.active = false;
    cls.socket.close();

    _world.reset();

    _menu_active = true;
}

//------------------------------------------------------------------------------
void session::start_client_local()
{
    cls.active = true;
    cls.local = true;
}

//------------------------------------------------------------------------------
void session::client_connectionless(network::address const& remote, network::message& message)
{
    string::view message_string(message.read_string());

    if (message_string.starts_with("info")) {
        info_get(remote, message_string);
    } else if (message_string.starts_with("connect")) {
        connect_ack(message_string);
    } else if (message_string.starts_with("fail")) {
        read_fail(message_string);
    }
}

//------------------------------------------------------------------------------
void session::client_packet(network::message& message)
{
    while (message.bytes_remaining()) {
        switch (message.read_byte()) {
            case svc_disconnect:
                write_message( "Disconnected from server." );
                stop_client();
                break;

            case svc_message:
                write_message(string::view(message.read_string()));
                break;

            case svc_info:
                read_info(message);
                break;

            case svc_snapshot:
                read_snapshot(message);
                break;

            case svc_restart:
                _restart_time = _frametime + time_delta::from_seconds(message.read_byte() * 1.0f);
                break;

            default:
                return;
        }
    }
}

//------------------------------------------------------------------------------
void session::read_snapshot(network::message& message)
{
    _world.read_snapshot(message);
    // gradually adjust client world time to match server
    // to compensate for variability in packet delivery
    _worldtime += (_world.frametime() - _worldtime) * 0.1f;
    _net_bytes[++_framenum % _net_bytes.size()] = 0;
}

//------------------------------------------------------------------------------
void session::connect_to_server (int index)
{
    // ask server for a connection

    stop_client( );
    stop_server( );

    for (word ii = 0; ii < 16; ++ii) {
        if (cls.socket.open(network::socket_type::ipv6, PORT_CLIENT+ii)) {
            break;
        }
    }

    if ( index > 0 )
        _netserver = cls.servers[index].address;

    if ( !_netserver.port )
        _netserver.port = PORT_SERVER;

    cls.socket.printf(_netserver, "connect %i %s %i", PROTOCOL_VERSION, cls.info.name.data(), _netchan.netport());
}

//------------------------------------------------------------------------------
void session::connect_to_server (string::view address)
{
    // ask server for a connection

    stop_client( );
    stop_server( );

    for (word ii = 0; ii < 16; ++ii) {
        if (cls.socket.open(network::socket_type::ipv6, PORT_CLIENT+ii)) {
            break;
        }
    }

    if (!cls.socket.resolve(address, _netserver)) {
        write_message_client(va("Failed to resolve address: \"%s\"", address));
        return;
    }

    if ( !_netserver.port )
        _netserver.port = PORT_SERVER;

    cls.socket.printf(_netserver, "connect %i %s %i", PROTOCOL_VERSION, cls.info.name.data(), _netchan.netport());
}

//------------------------------------------------------------------------------
void session::connect_ack(string::view message_string)
{
    // server has ack'd our connect

    sscanf(message_string, "connect %i %lld", &cls.number, reinterpret_cast<int64_t*>(&_worldtime));

    _netchan.setup( &cls.socket, _netserver );

    cls.active = true;

    _menu_active = false;

    _clients[0].usercmd_time = time_value::zero;

    svs.clients[cls.number].active = true;
    svs.clients[cls.number].info.name, cls.info.name;
    svs.clients[cls.number].info.color = cls.info.color;
    svs.clients[cls.number].info.weapon = cls.info.weapon;

    write_info(_netchan, cls.number);
}

//------------------------------------------------------------------------------
void session::client_send ()
{
    if (_frametime - _clients[0].usercmd_time < _clients[0].usercmd_rate) {
        return;
    }

    game::usercmd cmd = _clients[0].input.generate();
    _clients[0].usercmd_time = _frametime;

    _netchan.write_byte(clc_command);
    _netchan.write_vector(cmd.move);
    _netchan.write_vector(cmd.look);
    _netchan.write_byte(narrow_cast<uint8_t>(cmd.action));

    // check if user info has been changed
    if (!_menu_active) {
        if (strcmp(svs.clients[cls.number].info.name.data(), cls.info.name.data())
            || svs.clients[cls.number].info.color != cls.info.color
            || svs.clients[cls.number].info.weapon != cls.info.weapon) {

            strcpy(svs.clients[cls.number].info.name.data(), cls.info.name.data());
            svs.clients[cls.number].info.color = cls.info.color;
            svs.clients[cls.number].info.weapon = cls.info.weapon;
            write_info(_netchan, cls.number);
        }
    }
}

//------------------------------------------------------------------------------
void session::info_ask ()
{
    network::address    addr;

    for ( int i=0 ; i<MAX_SERVERS ; i++ )
    {
        cls.servers[i].name[0] = 0;
        cls.servers[i].active = false;
    }

    cls.ping_time = _frametime;

    stop_client( );
    stop_server( );

    for (word ii = 0; ii < 16; ++ii) {
        if (cls.socket.open(network::socket_type::ipv6, PORT_CLIENT+ii)) {
            break;
        }
    }

    //  ping master server
    if (cls.socket.resolve(_net_master, addr)) {
        if (addr.port == 0) {
            addr.port = PORT_SERVER;
        }
        cls.socket.printf(addr, "info");
    }

    //  ping local network
    addr.type = network::address_type::broadcast;
    addr.port = PORT_SERVER;

    cls.socket.printf(addr, "info");
}

//------------------------------------------------------------------------------
void session::info_get(network::address const& remote, string::view message_string)
{
    for (int ii=0; ii < MAX_SERVERS; ++ii) {
        // already exists and active, abort
        if ( cls.servers[ii].address == remote && cls.servers[ii].active )
            return;

        // slot taken, try next
        if ( cls.servers[ii].active )
            continue;

        cls.servers[ii].address = remote;
        cls.servers[ii].ping = _frametime - cls.ping_time;

        strcpy(cls.servers[ii].name, message_string.skip(5));

        cls.servers[ii].active = true;

        // old server code
        _netserver = remote;

        return;
    }
}

//------------------------------------------------------------------------------
void session::draw_world()
{
    //
    // calculate view
    //

    vec2 world_size = _world.maxs() - _world.mins();
    vec2 world_center = _world.mins() + world_size * 0.5f;

    render::view view{};
    view.size = world_size;

    if (cls.active && _player) {
        view.origin = _player->get_position(_worldtime);
        view.angle = _player->get_rotation(_worldtime);
    } else {
        view.origin = world_center;
        view.angle = 0;
    }

    view.size /= _zoom;
    // maintain aspect ratio
    view.size.x *= (view.size.y * _renderer->window()->width())
                 / (view.size.x * _renderer->window()->height());

    // draw world

    _renderer->set_view(view);

    if (cls.active) {
        _world.draw(_renderer, _worldtime);
    }

    // update sound listener

    pSound->set_listener(
        vec3(view.origin.x, view.origin.y, view.size.x * math::sqrt2<float>),
        vec3(0,0,-1), // forward
        vec3(1,0,0), // right
        vec3(0,1,0) // up
    );
}

} // namespace game
