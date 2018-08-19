// g_server.cpp
//

#include "precompiled.h"
#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
void session::start_server ()
{
    stop_client( );

    reset();

    for (std::size_t ii = 0; ii < svs.clients.size(); ++ii) {
        svs.clients[ii].active = false;
        svs.clients[ii].local = false;
    }

    // init local player

    if (!_dedicated) {
        svs.clients[0].active = true;
        svs.clients[0].local = true;

        svs.clients[0].info.name = cls.info.name;
        svs.clients[0].info.color = cls.info.color;
        svs.clients[0].info.weapon = cls.info.weapon;

        spawn_player(0);
    }

    _menu_active = false;

    svs.active = true;
    svs.local = false;
    _net_server_name = svs.name;

    svs.socket.open(network::socket_type::ipv6, PORT_SERVER);
    _netchan.setup(&svs.socket, network::address{});

    _net_bytes.fill(0);
}

//------------------------------------------------------------------------------
void session::start_server_local()
{
    stop_client();

    _worldtime = time_value::zero;

    svs.active = true;
    svs.local = true;

    // init local players
    for (std::size_t ii = 0; ii < svs.clients.size(); ++ii) {
        if (ii < 2) {
            svs.clients[ii].active = true;
            svs.clients[ii].local = true;

            snprintf(svs.clients[ii].info.name.data(),
                     svs.clients[ii].info.name.size(), "Player %zu", ii+1);
            svs.clients[ii].info.color = player_colors[ii];
            svs.clients[ii].info.weapon = weapon_type::cannon;
        } else {
            svs.clients[ii].active = false;
            svs.clients[ii].local = false;
        }
    }

    new_game();
}

//------------------------------------------------------------------------------
void session::stop_server ()
{
    if (!svs.active) {
        return;
    }

    svs.active = false;
    svs.local = false;

    for (std::size_t ii = 0; ii < svs.clients.size(); ++ii) {
        if (svs.clients[ii].local || !svs.clients[ii].active) {
            continue;
        }

        client_disconnect(ii);

        svs.clients[ii].netchan.write_byte(svc_disconnect);
        svs.clients[ii].netchan.transmit();
        svs.clients[ii].netchan.reset();
    }

    svs.socket.close();

    _world.clear();
}

//------------------------------------------------------------------------------
void session::server_connectionless(network::address const& remote, network::message& message)
{
    string::view message_string(message.read_string());

    if (message_string.starts_with("info")) {
        info_send(remote);
    } else if (message_string.starts_with("connect")) {
        client_connect(remote, message_string);
    }
}

//------------------------------------------------------------------------------
void session::server_packet(network::message& message, std::size_t client)
{
    while (message.bytes_remaining()) {
        switch (message.read_byte()) {
            case clc_command:
                client_command(message, client);
                break;

            case clc_disconnect:
                write_message(va("%s disconnected.", svs.clients[client].info.name.data() ));
                client_disconnect(client);
                break;

            case clc_say:
                write_message(va( "^%x%x%x%s^xxx: %s",
                    (int )(svs.clients[client].info.color.r * 15.5f),
                    (int )(svs.clients[client].info.color.g * 15.5f),
                    (int )(svs.clients[client].info.color.b * 15.5f),
                    svs.clients[client].info.name.data(), message.read_string()));
                break;

            case svc_info:
                read_info(message);
                break;

            default:
                return;
        }
    }
}

//------------------------------------------------------------------------------
void session::write_frame()
{
    network::message_storage message;

    // check if local user info has been changed
    if (!_menu_active && svs.clients[0].local) {
        if (strcmp(svs.clients[0].info.name.data(), cls.info.name.data())
            || svs.clients[0].info.color != cls.info.color
            || svs.clients[0].info.weapon != cls.info.weapon) {

            strcpy(svs.clients[0].info.name.data(), cls.info.name.data());
            svs.clients[0].info.color = cls.info.color;
            svs.clients[0].info.weapon = cls.info.weapon;

            write_info(message, 0);
        }
    }


    _world.write_snapshot(message);

    broadcast(message);
}

//------------------------------------------------------------------------------
void session::client_connect(network::address const& remote, string::view message_string)
{
    // client has asked for connection
    if (!svs.active) {
        return;
    }

    // ensure that this client hasn't already connected
    for (auto const& cl : svs.clients) {
        if (cl.active && cl.netchan.address() == remote) {
            return;
        }
    }

    // find an available client slot
    for (std::size_t ii = 0; ii < svs.clients.size(); ++ii) {
        if (!svs.clients[ii].active) {
            return client_connect(remote, message_string, ii);
        }
    }

    svs.socket.printf(remote, "fail \"Server is full\"");
}

//------------------------------------------------------------------------------
void session::client_connect(network::address const& remote, string::view message_string, std::size_t client)
{
    auto& cl = svs.clients[client];
    int netport, version;

    sscanf(message_string, "connect %i %s %i", &version, cl.info.name.data(), &netport);

    if (version != PROTOCOL_VERSION) {
        svs.socket.printf(remote, "fail \"Bad protocol version: %i\"", version);
    } else {
        cl.active = true;
        cl.local = false;
        cl.netchan.setup(&svs.socket, remote, narrow_cast<word>(netport));

        svs.socket.printf(cl.netchan.address(), "connect %i %lld", client, _worldtime.to_microseconds());

        // init their tank

        spawn_player(client);

        write_message(va("%s connected.", cl.info.name.data()));

        // broadcast existing client information to new client
        for (std::size_t ii = 0; ii < svs.clients.size(); ++ii) {
            if (&cl != &svs.clients[ii]) {
                write_info(cl.netchan, ii);
            }
        }
    }
}

//------------------------------------------------------------------------------
void session::client_disconnect(std::size_t client)
{
    network::message_storage message;

    if (!svs.clients[client].active) {
        return;
    }

    svs.clients[client].active = false;

    write_info(message, client);
    broadcast(message);
}

//------------------------------------------------------------------------------
void session::client_command(network::message& message, std::size_t /*client*/)
{
    game::usercmd cmd{};

    cmd.move = message.read_vector();
    cmd.look = message.read_vector();
    cmd.action = static_cast<decltype(cmd.action)>(message.read_byte());

    //game::tank* player = _world.player(client);
    //if (player) {
    //    player->update_usercmd(cmd);
    //}
}

//------------------------------------------------------------------------------
void session::info_send(network::address const& remote)
{
    int     i;

    if ( !svs.active )
        return;

    // check for an empty slot
    for ( i=0 ; i<MAX_PLAYERS ; i++ )
        if ( !svs.clients[i].active )
            break;

    // full, shhhhh
    if ( i == MAX_PLAYERS )
        return;

    svs.socket.printf(remote, "info %s", svs.name);
}

} // namespace game
