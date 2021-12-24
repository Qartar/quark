// net_message.h
//

#pragma once

#include "cm_shared.h"

#include <climits>
#include <array>

////////////////////////////////////////////////////////////////////////////////
namespace network {

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
    //! Read a delta-compressed message from `reader` using `source` as the base, optionally write uncompressed message to `target`
    static delta_message read(network::message const* source, network::message const* reader, network::message* target) {
        return delta_message(source, reader, target);
    }

    //! Write a delta-compressed message to `writer` using `source` as the base, optionally write uncompressed message to `target`
    static delta_message write(network::message const* source, network::message* writer, network::message* target) {
        return delta_message(source, writer, target);
    }

    //! true if source is different than target
    bool has_changed() const { return _has_changed; }

    //! write an arbitrary number of bits
    void write_bits(int value, int bits);
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

    //! read an arbitrary number of bits
    int read_bits(int bits) const;
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

    //! write a 32-bit float with an expected change of `delta`
    void write_delta_float(float value, float delta, float epsilon = 1e-3f);
    //! write a two-dimensional vector with an expected change of `delta`
    //!     e.g. write_delta_vector(origin, velocity)
    void write_delta_vector(vec2 value, vec2 delta, float epsilon = 1e-3f);

    //! read a 32-bit float with an expected change of `delta`
    float read_delta_float(float delta) const;
    //! read a two-dimensional vector with an expected change of `delta`
    //!     e.g. origin = read_delta_vector(velocity)
    vec2 read_delta_vector(vec2 delta) const;

    //! number of bytes that can be read
    std::size_t bytes_remaining() const { return _reader ? _reader->bytes_remaining() : 0; }
    //! number of bytes that can be written or reserved
    std::size_t bytes_available() const { return _writer ? _writer->bytes_available() : 0; }

protected:
    network::message const* _source;
    network::message const* _reader;
    network::message* _writer;
    network::message* _target; //!< source + delta
    mutable bool _has_changed;

private:
    delta_message(network::message const* source, network::message const* reader, network::message* target);
    delta_message(network::message const* source, network::message* writer, network::message* target);
};

} // namespace network
