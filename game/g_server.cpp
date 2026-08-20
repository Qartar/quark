// g_server.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_parser.h"
#include "g_faction.h"
#include "g_formation.h"
#include "g_globe.h"
#include "g_navigation.h"
#include "g_player.h"
#include "g_ship.h"
#include "design/g_ship_design.h"
#include "design/g_design_manager.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
void session::command_start(parser::text const& args)
{
    stop_server();
    stop_client();

    start_server_local();
    start_client_local();

    start_game(args);
    _console.deactivate();
}

//------------------------------------------------------------------------------
void session::start_game(parser::text const& args)
{
    if (!svs.active) {
        return;
    }

    //
    //  reset players
    //

    for ( int i=0 ; i<MAX_PLAYERS ; i++ )
    {
        if (svs.local && i > 1 )
            break;
        else if (svs.active && !svs.clients[i].active )
            continue;
    }

    _menu_active = false;

    //
    //  reset world
    //

    _world.reset( );
    _worldtime = time_value::zero;
    _player = _world.spawn<player>();

    enum class scenario {
        battle,
        lineup,
    } scenario = scenario::battle;

    vec3 origin = globe::lonlat_to_surface(vec2(117.9167, -1.95) * (math::pi / 180.0)); // Makassar Strait

    std::vector<ship_design const*> ship_designs = {
        // Fast battleships
        _world.designs().find_ship("yamato_battleship"),
        _world.designs().find_ship("north_carolina_battleship"),
        _world.designs().find_ship("king_george_v_battleship"),
        _world.designs().find_ship("richelieu_battleship"),
        _world.designs().find_ship("bismarck_battleship"),
        _world.designs().find_ship("littorio_battleship"),
        // Heavy cruisers
        _world.designs().find_ship("mogami_cruiser"),
        _world.designs().find_ship("new_orleans_cruiser"),
        _world.designs().find_ship("county_cruiser"),
        _world.designs().find_ship("algerie_cruiser"),
        _world.designs().find_ship("admiral_hipper_cruiser"),
        _world.designs().find_ship("zara_cruiser"),
    };

    for (std::size_t ii = 1; ii < args.tokens().size(); ++ii) {

        //
        // Location arguments
        //

        if (args.tokens()[ii] == "makassar") {
            // Makassar Strait
            origin = globe::lonlat_to_surface(vec2(117.9167, -1.95) * (math::pi / 180.0));
        } else if (args.tokens()[ii] == "otranto") {
            // Strait of Otranto
            origin = globe::lonlat_to_surface(vec2(18.8, 39.9) * (math::pi / 180.0));

        //
        // Ships arguments
        //

        } else if (args.tokens()[ii] == "battleship") {
            // Fast battleships
            ship_designs = {
                _world.designs().find_ship("yamato_battleship"),
                _world.designs().find_ship("north_carolina_battleship"),
                _world.designs().find_ship("king_george_v_battleship"),
                _world.designs().find_ship("richelieu_battleship"),
                _world.designs().find_ship("bismarck_battleship"),
                _world.designs().find_ship("littorio_battleship"),
            };
        } else if (args.tokens()[ii] == "cruiser") {
            // Heavy cruisers
            ship_designs = {
                _world.designs().find_ship("mogami_cruiser"),
                _world.designs().find_ship("new_orleans_cruiser"),
                _world.designs().find_ship("county_cruiser"),
                _world.designs().find_ship("algerie_cruiser"),
                _world.designs().find_ship("admiral_hipper_cruiser"),
                _world.designs().find_ship("zara_cruiser"),
            };
        } else if (args.tokens()[ii] == "all") {
            // All ship designs
            ship_designs = {
                _world.designs().find_ship("yamato_battleship"),
                _world.designs().find_ship("fuso_battleship"),
                _world.designs().find_ship("iowa_battleship"),
                _world.designs().find_ship("north_carolina_battleship"),
                _world.designs().find_ship("king_george_v_battleship"),
                _world.designs().find_ship("richelieu_battleship"),
                _world.designs().find_ship("bismarck_battleship"),
                _world.designs().find_ship("littorio_battleship"),
                _world.designs().find_ship("deutschland_cruiser"),
                _world.designs().find_ship("mogami_cruiser"),
                _world.designs().find_ship("new_orleans_cruiser"),
                _world.designs().find_ship("county_cruiser"),
                _world.designs().find_ship("algerie_cruiser"),
                _world.designs().find_ship("admiral_hipper_cruiser"),
                _world.designs().find_ship("zara_cruiser"),
                _world.designs().find_ship("town_cruiser"),
                _world.designs().find_ship("tribal_destroyer"),
            };

        //
        // Scenario arguments
        //

        } else if (args.tokens()[ii] == "battle") {
            scenario = scenario::battle;
        } else if (args.tokens()[ii] == "lineup") {
            scenario = scenario::lineup;

        // Unknown argument
        } else {
            warning("ignoring unknown argument: '^fff%s^xxx'\n", args.tokens()[ii].c_str());
        }
    }

    _player->set_position(origin);

    if (scenario == scenario::battle) {
        faction* blufor = _world.spawn<faction>("blufor", color4(.6f, .8f, 1.f, 1.f));
        faction* opfor = _world.spawn<faction>("opfor", color4(1.f, .6f, .6f, 1.f));

        formation* blueform = _world.spawn<formation>();
        formation* opform = _world.spawn<formation>();

        constexpr int N = 6;
        vec2 offset[N];
        vec3 position[N];
        for (int ii = 0; ii < N; ++ii) {
            double angle = double(ii) * (math::pi * 2.0 / double(N)) + math::pi / double(2 * N);
            offset[ii] = vec2(std::cos(angle), std::sin(angle)) * -1024;
        }

        globe::offset(origin, offset, position);
        for (int ii = 0; ii < N; ++ii) {
            ship* sh = _world.spawn<ship>(ship_designs[ii % ship_designs.size()], blufor);
            sh->set_position(position[ii], true);
            sh->set_heading(rot2(0,1), true);
            sh->navigation()->set_heading(rot2(0,1));
            blueform->add(sh);
        }

        vec3 origin2 = globe::offset(origin, vec2(16384,0));
        globe::offset(origin2, offset, position);
        for (int ii = 0; ii < N; ++ii) {
            ship* sh = _world.spawn<ship>(ship_designs[ii % ship_designs.size()], opfor);
            sh->set_position(position[ii], true);
            sh->set_heading(rot2(0,1), true);
            sh->navigation()->set_heading(rot2(0,1));
            opform->add(sh);
        }
    } else if (scenario == scenario::lineup) {
        faction* blufor = _world.spawn<faction>("blufor", color4(.6f, .8f, 1.f, 1.f));
        constexpr std::size_t N = 256;
        assert(N >= ship_designs.size());
        vec2 offset[N];
        for (int ii = 0; ii < N; ++ii) {
            offset[ii] = vec2(ii * 100 - 250, 0);
        }
        vec3 position[N];
        globe::offset(origin, offset, position, min(N, ship_designs.size()));

        for (std::size_t ii = 0, sz = min(N, ship_designs.size()); ii < sz; ++ii) {
            ship* sh = _world.spawn<ship>(ship_designs[ii], blufor);
            sh->set_position(position[ii], true);
            sh->set_heading(rot2(0,1), true);
            sh->navigation()->set_heading(rot2(0,1));
        }
    }
}

//------------------------------------------------------------------------------
void session::start_server ()
{
    stop_client( );

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
    }

    _menu_active = false;

    svs.active = true;
    svs.local = false;

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
        } else {
            svs.clients[ii].active = false;
            svs.clients[ii].local = false;
        }
    }
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
            || svs.clients[0].info.color != cls.info.color) {

            strcpy(svs.clients[0].info.name, string::view(cls.info.name.data()));
            svs.clients[0].info.color = cls.info.color;

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

    cmd.cursor = message.read_vector();
    cmd.action = static_cast<decltype(cmd.action)>(message.read_byte());
    cmd.buttons = static_cast<decltype(cmd.buttons)>(message.read_byte());
    cmd.modifiers = static_cast<decltype(cmd.modifiers)>(message.read_byte());
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
