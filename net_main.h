// net_main.h
//

#pragma once

#include "shared.h"

#include <climits>
#include <array>

struct sockaddr_storage;

////////////////////////////////////////////////////////////////////////////////
namespace network {

enum address_type { loopback, broadcast, ipv4, ipv6 };

//------------------------------------------------------------------------------
class address
{
public:
    address_type type;

    union {
        std::array<byte, 4> ip4;
        std::array<word, 8> ip6;
    };

    unsigned short port;

public:
    address() = default;
    constexpr address(byte const (&ip)[4], word port = 0)
        : type(address_type::ipv4)
        , ip4({ip[0], ip[1], ip[2], ip[3]})
        , port(port)
    {}
    constexpr address(word const (&ip)[8], word port = 0)
        : type(address_type::ipv6)
        , ip6({ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7]})
        , port(port)
    {}

    bool operator==(network::address const& other) const;
    bool operator!=(network::address const& other) const { return !(*this == other); }
    bool is_local() const { return (type == network::address_type::loopback); }
};

//------------------------------------------------------------------------------
class message
{
public:
    message(byte* data, std::size_t size);

    //! reset write and read cursor to beginning of internal buffer
    void reset();
    //! reset read cursor to beginning of internal buffer
    void rewind() const;
    //! move read cursor back by the given number of bytes
    void rewind(std::size_t size) const;
    //! move read cursor back by the given number of bits
    void rewind_bits(std::size_t bits) const;

    //! return a pointer to internal buffer at current write cursor for writing
    byte* write(std::size_t size);
    //! return a pointer to internal buffer at current read cursor for reading
    byte const* read(std::size_t size) const;

    //! reserve bytes for writing but does not advance write cursor until commit
    byte* reserve(std::size_t size);
    //! commit reserved bytes, size must be less than or equal to total reserved bytes
    std::size_t commit(std::size_t size);

    //! write data from the given buffer
    std::size_t write(byte const* data, std::size_t size);
    //! read data into the given buffer
    std::size_t read(byte* data, std::size_t size) const;

    //! write data from the given message
    std::size_t write(network::message const& message);
    //! read data into the given message
    std::size_t read(network::message& message) const;

    //! number of bits that have been read
    std::size_t bits_read() const;
    //! number of bits that have been written
    std::size_t bits_written() const;

    //! number of bits that can be read
    std::size_t bits_remaining() const;
    //! number of bits than can be written
    std::size_t bits_available() const;

    //! number of bytes that have been read
    std::size_t bytes_read() const { return _bytes_read; }
    //! number of bytes that have been written
    std::size_t bytes_written() const { return _bytes_written; }

    //! number of bytes that can be read
    std::size_t bytes_remaining() const { return _bytes_written - _bytes_read; }
    //! number of bytes that can be written or reserved
    std::size_t bytes_available() const { return _size - _bytes_written - _bytes_reserved; }

    //! write an arbitrary number of bits
    void write_bits(int value, int bits);
    //! write bit padding up to the next aligned byte
    void write_align();
    //! write an 8-bit unsigned integer
    void write_byte(int b) { write_bits(b, 8); }
    //! write an 8-bit signed integer
    void write_char(int c) { write_bits(c, -8); }
    //! write a 16-bit signed integer
    void write_short(int s) { write_bits(s, -16); }
    //! write a 32-bit signed integer
    void write_long(int l) { write_bits(l, -32); }
    //! write a 32-bit float
    void write_float(float f);
    //! write a two-dimensional vector
    void write_vector(vec2 v);
    //! write a null-terminated string
    void write_string(char const* sz);

    //! read an arbitrary number of bits
    int read_bits(int bits) const;
    //! read bit padding up to the next aligned byte
    void read_align() const;
    //! read an 8-bit unsigned integer
    int read_byte() const { return read_bits(8); }
    //! read an 8-bit signed integer
    int read_char() const { return read_bits(-8); }
    //! read a 16-bit signed integer
    int read_short() const { return read_bits(-16); }
    //! read a 32-bit signed integer
    int read_long() const { return read_bits(-32); }
    //! read a 32-bit float
    float read_float() const;
    //! read a two-dimensional vector
    vec2 read_vector() const;
    //! read a null-terminated string
    char const* read_string() const;

protected:
    byte* _data;
    std::size_t _size;
    mutable int _bits_read;
    int _bits_written;
    mutable std::size_t _bytes_read;
    std::size_t _bytes_written;
    std::size_t _bytes_reserved;

    constexpr static int byte_bits = CHAR_BIT;

protected:
    message(message&&) = delete;
    message& operator=(message&&) = delete;
};

//------------------------------------------------------------------------------
class message_storage : public message
{
public:
    message_storage()
        : message(_buffer.data(), _buffer.size())
    {}

protected:
    std::array<byte, 1400> _buffer;
};

//------------------------------------------------------------------------------
class delta_message
{
public:
    //! write an arbitrary number of bits
    void write_bits(int value, int bits);

    //! read an arbitrary number of bits
    int read_bits(int bits) const;

protected:
    network::message const* _source;
    network::message const* _reader;
    network::message* _writer;
    network::message* _target; //!< source + delta
};

//------------------------------------------------------------------------------
enum class socket_type { unspecified, ipv4, ipv6 };
enum socket_port { any = 0 };

//------------------------------------------------------------------------------
class socket
{
public:
    socket();
    socket(socket_type type, word port = socket_port::any);
    ~socket();

    socket(socket&&);
    socket& operator=(socket&&);

    socket_type type() const { return _type; }
    socket_port port() const { return _port; }

    bool valid() const { return _socket != 0; }
    bool open(socket_type type, word port = socket_port::any);
    void close();

    //! read data info message and set remote address
    bool read(network::address& remote, network::message& message);
    //! write data to the remote address
    bool write(network::address const& remote, network::message const& message);
    //! write a formatted string to the remote address
    bool printf(network::address const& remote, char const* fmt, ...);

    //! resolve the string into an address
    bool resolve(std::string const& address_string, network::address& address) const;

protected:
    socket_type _type;
    socket_port _port;
    std::uintptr_t _socket;

protected:
    socket(socket const&) = delete;
    socket& operator=(socket const&) = delete;

    bool sockaddr_to_address(sockaddr_storage const& sockaddr, network::address& address) const;
    bool address_to_sockaddr(network::address const& address, sockaddr_storage& sockaddr) const;
    bool resolve_sockaddr(std::string const& address_string, sockaddr_storage& sockaddr) const;

    std::uintptr_t open_socket(socket_type type, word port = socket_port::any) const;
};

//------------------------------------------------------------------------------
class channel : public message_storage
{
public:
    constexpr static int prefix = -1;

public:
    channel(int netport = 0);

    //! set up channel with the remote address over the given network socket
    void setup(network::socket* socket, network::address remote, int netport = 0);

    //! transmit accumulated message to remote address
    bool transmit();

    //! process incoming message
    bool process(network::message& message);

    //! remote address
    network::address const& address() const { return _address; }

    //! network port for address translation
    word netport() const { return _netport; }

    //! time of most recently transmitted message
    float last_send() const { return _last_sent; }

    //! time of most recently processed message
    float last_received() const { return _last_received; }

protected:
    network::address _address; //!< remote address
    word _netport; //!< port translation

    float _last_sent; //!< time of most recently transmitted message
    float _last_received; //!< time of most recently processed message

    network::socket* _socket; //!< socket used for transmitting data

protected:
    bool transmit(int length, byte const* data);
};

} // namespace network
