// g_network.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_parser.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
void session::get_packets ()
{
    network::socket* socket = svs.active ? &svs.socket : &cls.socket;
    network::message_storage message;
    network::address remote;

    while (socket->read(remote, message)) {
        _net_bytes[_framenum % _net_bytes.size()] += message.bytes_remaining();

        int prefix = message.read_long();
        if (prefix != network::channel::prefix) {
            message.rewind();
            if (socket == &svs.socket) {
                server_connectionless(remote, message);
            } else {
                client_connectionless(remote, message);
            }
            message.reset();
            continue;
        }

        if (socket == &svs.socket) {
            int netport = (word )message.read_short();

            for (std::size_t ii = 0; ii < svs.clients.size(); ++ii) {
                if (svs.clients[ii].local)
                    continue;
                if (!svs.clients[ii].active)
                    continue;
                if (svs.clients[ii].netchan.address() != remote)
                    continue;
                if (svs.clients[ii].netchan.netport() != netport)
                    continue;

                // found him
                svs.clients[ii].netchan.process(message);
                server_packet(message, ii);
                break;
            }
        } else {
            message.read_short(); // skip netport

            if (remote != _netserver) {
                break;  // not from our server
            }
            _netchan.process(message);
            client_packet(message);
        }

        message.reset();
    }

    //
    // check for timeouts
    //

    // network uses application time directly so do the same here
    time_value time = time_value::current();
    constexpr time_delta timeout = time_delta::from_seconds(10);

    if (svs.active) {
        for (std::size_t ii = 0; ii < svs.clients.size(); ++ii) {
            if (svs.clients[ii].local || !svs.clients[ii].active) {
                continue;
            }

            if (svs.clients[ii].netchan.last_received() + timeout < time) {
                svs.clients[ii].netchan.write_byte(svc_disconnect);
                svs.clients[ii].netchan.transmit();

                write_message(va("%s timed out.", svs.clients[ii].info.name.data()));
                client_disconnect(ii);
            }
        }
    } else if (cls.active) {
        if (_netchan.last_received() + timeout < time) {
            write_message("Server timed out.");
            stop_client();
        }
    }
}

//------------------------------------------------------------------------------
void session::broadcast(std::size_t len, byte const* data)
{
    for (auto& cl : svs.clients) {
        if (!cl.local && cl.active) {
            cl.netchan.write(data, len);
        }
    }
}

//------------------------------------------------------------------------------
void session::broadcast(network::message& message)
{
    std::size_t length = message.bytes_remaining();
    byte const* data = message.read(length);

    broadcast(length, data);
}

//------------------------------------------------------------------------------
void session::broadcast_print (string::view message)
{
    network::message_storage netmsg;
        
    netmsg.write_byte( svc_message );
    netmsg.write_string( message.c_str() );

    broadcast(netmsg);
}

//------------------------------------------------------------------------------
void session::send_packets ()
{
    if (svs.active) {
        for (auto& cl : svs.clients) {
            if (cl.local || !cl.active || !cl.netchan.bytes_remaining()) {
                continue;
            }

            cl.netchan.transmit();
            cl.netchan.reset();
        }
    } else if (cls.active) {
        client_send();

        if (_netchan.bytes_remaining()) {
            _netchan.transmit();
            _netchan.reset();
        }
    }
}

//------------------------------------------------------------------------------
void session::read_fail(string::view message_string)
{
    parser::text text(message_string);
    if (text.tokens().size() > 1) {
        write_message(va("Failed to connect: %s", text.tokens()[1]));
    } else {
        write_message("Failed to connect");
    }
}

//------------------------------------------------------------------------------
void session::write_info(network::message& message, std::size_t client)
{
    message.write_byte( svc_info );
    message.write_byte( narrow_cast<uint8_t>(client) );
    message.write_byte( svs.clients[client].active );
    message.write_string( svs.clients[client].info.name.data() );

    message.write_float( svs.clients[client].info.color.r );
    message.write_float( svs.clients[client].info.color.g );
    message.write_float( svs.clients[client].info.color.b );

    message.write_byte( narrow_cast<uint8_t>(svs.clients[client].info.weapon ) );
}

//------------------------------------------------------------------------------
void session::read_info(network::message& message)
{
    int client;
    int active;
    char const* string;

    client = message.read_byte();
    active = message.read_byte();

    svs.clients[client].active = (active == 1);

    string = message.read_string();
    strncpy(svs.clients[client].info.name.data(), string, SHORT_STRING);

    svs.clients[client].info.color.r = message.read_float();
    svs.clients[client].info.color.g = message.read_float();
    svs.clients[client].info.color.b = message.read_float();

    svs.clients[client].info.weapon = static_cast<weapon_type>(message.read_byte());

    // relay info to other clients
    if (svs.active) {
        network::message_storage netmsg;

        write_info(netmsg, client);
        broadcast(netmsg);
    }
}

} // namespace game
