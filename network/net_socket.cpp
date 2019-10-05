// net_sock.cpp
//

#include "net_socket.h"
#include "net_address.h"
#include "net_message.h"

#if defined(_WIN32)
#   include <WS2tcpip.h>
#else // !defined(_WIN32)
#   include <unistd.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netinet/ip.h>
#endif // !defined(_WIN32)

////////////////////////////////////////////////////////////////////////////////
namespace network {

//------------------------------------------------------------------------------
socket::socket()
    : _type(socket_type::unspecified)
    , _port(any)
    , _socket(0)
{}

//------------------------------------------------------------------------------
socket::socket(socket_type type, word port)
    : _type(type)
    , _port(static_cast<socket_port>(port))
    , _socket(open_socket(type, port))
{}

//------------------------------------------------------------------------------
socket::~socket()
{
    close();
}

//------------------------------------------------------------------------------
socket::socket(socket&& other)
    : _type(other._type)
    , _port(other._port)
    , _socket(other._socket)
{
    other._socket = 0;
}

//------------------------------------------------------------------------------
socket& socket::operator=(socket&& other)
{
    close();

    _type = other._type;
    _port = other._port;
    _socket = other._socket;

    other._socket = 0;
    return *this;
}

//------------------------------------------------------------------------------
bool socket::open(socket_type type, word port)
{
    close();

    _type = type;
    _port = static_cast<socket_port>(port);
    _socket = open_socket(type, port);

    return _socket != 0;
}

//------------------------------------------------------------------------------
std::uintptr_t socket::open_socket(socket_type type, word port) const
{
#if defined(_WIN32)
    std::uintptr_t newsocket = 0;
    unsigned long args = 1;

    int family = (type == socket_type::ipv4) ? PF_INET :
                 (type == socket_type::ipv6) ? PF_INET6 : PF_UNSPEC;

    // get socket
    if ( (newsocket = ::socket(family, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        return 0;
    }

    // disable blocking
    if (ioctlsocket(newsocket, FIONBIO, &args) == SOCKET_ERROR) {
        ::closesocket(newsocket);
        return 0;
    }

    // enable broadcasting
    if (type == socket_type::ipv4) {
        if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char const*)&args, sizeof(args) ) == SOCKET_ERROR) {
            ::closesocket(newsocket);
            return 0;
        }

    } else if (type == socket_type::ipv6) {
        ipv6_mreq mreq = {};

        mreq.ipv6mr_multiaddr = in6addr_allnodesonlink;
        mreq.ipv6mr_interface = 0;

        //  add membership to link-local multicast group
        if (setsockopt(newsocket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char const*)&mreq, sizeof(mreq)) == SOCKET_ERROR) {
            ::closesocket(newsocket);
            return 0;
        }
    }

    // bind socket
    if (type == socket_type::ipv4) {
        sockaddr_in address = {};

        address.sin_port = htons(port);
        address.sin_addr = in4addr_any;
        address.sin_family = AF_INET;

        if (bind(newsocket, (sockaddr *)&address, sizeof(address)) != SOCKET_ERROR) {
            return newsocket;
        }

    } else if (type == socket_type::ipv6) {
        sockaddr_in6 address = {};

        address.sin6_family = AF_INET6;
        address.sin6_addr = in6addr_any;
        address.sin6_port = htons(port);

        if (bind(newsocket, (sockaddr *)&address, sizeof(address)) != SOCKET_ERROR) {
            return newsocket;
        }
    }

    closesocket(newsocket);
    return false;
#else // !defined(_WIN32)
    return 0;
#endif // !defined(_WIN32)
}

//------------------------------------------------------------------------------
void socket::close()
{
    if (_socket) {
#if defined(_WIN32)
        ::closesocket(_socket);
#else // !defined(_WIN32)
        ::close(_socket);
#endif // !defined(_WIN32)
        _socket = 0;
    }
}

//------------------------------------------------------------------------------
bool socket::read(network::address& remote, network::message& message)
{
    sockaddr_storage from = {};
    socklen_t fromlen = sizeof(from);

    if (!_socket) {
        return false;
    }

    std::size_t len = message.bytes_available();
    char* buf = (char*)message.reserve(len);

    int result = ::recvfrom(_socket, buf, narrow_cast<int>(len), 0, (sockaddr*)&from, &fromlen);

    if (result <= 0) {
        return false;
    }

    if (!sockaddr_to_address(from, remote)) {
        return false;
    }

    message.commit(result);
    return true;
}

//------------------------------------------------------------------------------
bool socket::write(network::address const& remote, network::message const& message)
{
    sockaddr_storage to = {};
    int tolen = sizeof(to);

    if (!_socket) {
        return false;
    }

    if (!address_to_sockaddr(remote, to)) {
        return false;
    }

    std::size_t len = message.bytes_remaining();
    char const* buf = (char const*)message.read(len);

    int result = ::sendto(_socket, buf, narrow_cast<int>(len), 0, (sockaddr*)&to, tolen);

    if (result < narrow_cast<int>(len)) {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool socket::printf(network::address const& remote, string::literal fmt, ...)
{
    network::message_storage message;

    va_list va;

    std::size_t size = message.bytes_available();
    byte* buf = message.reserve(size);

    va_start(va, fmt);
    int len = vsnprintf((char*)buf, size, fmt.c_str(), va);
    va_end(va);

    if (len >= 0) {
        message.commit(len + 1);
        return write(remote, message);
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool socket::resolve(string::view address_string, network::address& address) const
{
    if (address_string == "localhost") {
        address = {};
        address.type = address_type::loopback;
        return true;
    } else {
        sockaddr_storage sockaddr = {};

        if (resolve_sockaddr(address_string, sockaddr)) {
            return sockaddr_to_address(sockaddr, address);
        }
    }

    return false;
}

//------------------------------------------------------------------------------
bool socket::sockaddr_to_address(sockaddr_storage const& sockaddr, network::address& address) const
{
#if defined(_WIN32)
    if (sockaddr.ss_family == AF_INET) {
        auto const& sockaddr_ipv4 = reinterpret_cast<sockaddr_in const&>(sockaddr);

        if (_type == socket_type::ipv4 || _type == socket_type::unspecified) {
            address.type = network::address_type::ipv4;
            address.port = ntohs(sockaddr_ipv4.sin_port);
            *(uint32_t*)address.ip4.data() = sockaddr_ipv4.sin_addr.s_addr;
        } else if (_type == socket_type::ipv6) {
            // IPv4-mapped IPv6 address
            address.type = network::address_type::ipv6;
            address.port = ntohs(sockaddr_ipv4.sin_port);
            address.ip6.fill(0);
            address.ip6[5] = 0xffff;
            address.ip6[6] = ntohs(sockaddr_ipv4.sin_addr.S_un.S_un_w.s_w1);
            address.ip6[7] = ntohs(sockaddr_ipv4.sin_addr.S_un.S_un_w.s_w2);
        } else {
            return false;
        }
    } else if (sockaddr.ss_family == AF_INET6) {
        auto const& sockaddr_ipv6 = reinterpret_cast<sockaddr_in6 const&>(sockaddr);

        if (_type == socket_type::ipv6 || _type == socket_type::unspecified) {
            address.type = network::address_type::ipv6;
            address.port = ntohs(sockaddr_ipv6.sin6_port);
            for (std::size_t ii = 0; ii < address.ip6.size(); ++ii) {
                address.ip6[ii] = ntohs(sockaddr_ipv6.sin6_addr.u.Word[ii]);
            }
        } else {
            return false;
        }
    }

    return true;
#else // !defined(_WIN32)
    return false;
#endif // !defined(_WIN32)
}

//------------------------------------------------------------------------------
bool socket::address_to_sockaddr(network::address const& address, sockaddr_storage& sockaddr) const
{
#if defined(_WIN32)
    sockaddr = {};

    if (_type == socket_type::ipv4) {
        auto& sockaddr_ipv4 = reinterpret_cast<sockaddr_in&>(sockaddr);

        sockaddr_ipv4.sin_family = AF_INET;
        sockaddr_ipv4.sin_port = htons(address.port);

        if (address.type == network::address_type::loopback) {
            sockaddr_ipv4.sin_addr = in4addr_loopback;
        } else if (address.type == network::address_type::broadcast) {
            sockaddr_ipv4.sin_addr = in4addr_broadcast;
        } else if (address.type == network::address_type::ipv4) {
            sockaddr_ipv4.sin_addr.s_addr = *(uint32_t*)address.ip4.data();
        } else {
            return false;
        }
    } else if (_type == socket_type::ipv6 || _type == socket_type::unspecified) {
        auto& sockaddr_ipv6 = reinterpret_cast<sockaddr_in6&>(sockaddr);

        sockaddr_ipv6.sin6_family = AF_INET6;
        sockaddr_ipv6.sin6_port = htons(address.port);

        if (address.type == network::address_type::loopback) {
            sockaddr_ipv6.sin6_addr = in6addr_loopback;
        } else if (address.type == network::address_type::broadcast) {
            sockaddr_ipv6.sin6_addr = in6addr_allnodesonlink;
        } else if (address.type == network::address_type::ipv4) {
            // IPv4-mapped IPv6 address
            sockaddr_ipv6.sin6_addr.u.Word[ 5] = 0xffff;
            sockaddr_ipv6.sin6_addr.u.Byte[12] = address.ip4[0];
            sockaddr_ipv6.sin6_addr.u.Byte[13] = address.ip4[1];
            sockaddr_ipv6.sin6_addr.u.Byte[14] = address.ip4[2];
            sockaddr_ipv6.sin6_addr.u.Byte[15] = address.ip4[3];
        } else if (address.type == network::address_type::ipv6) {
            for (size_t ii = 0; ii < address.ip6.size(); ++ii) {
                sockaddr_ipv6.sin6_addr.u.Word[ii] = htons(address.ip6[ii]);
            }
        } else {
            return false;
        }
    } else {
        return false;
    }

    return true;
#else // !defined(_WIN32)
    return false;
#endif // !defined(_WIN32)
}

//------------------------------------------------------------------------------
bool socket::resolve_sockaddr(string::view address_string, sockaddr_storage& sockaddr) const
{
#if defined(_WIN32)
    addrinfo* info = nullptr;

    addrinfo hints = {};

    hints.ai_family = (_type == socket_type::ipv4) ? PF_INET :
                      (_type == socket_type::ipv6) ? PF_INET6 : PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(address_string.c_str(), nullptr, &hints, &info) == 0) {
        memcpy(&sockaddr, info->ai_addr, info->ai_addrlen);
        freeaddrinfo(info);
        return true;
    }
#endif // defined(_WIN32)

    return false;
}

} // namespace network
