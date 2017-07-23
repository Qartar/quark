// net_msg.cpp
//

#include "net_message.h"

////////////////////////////////////////////////////////////////////////////////
namespace network {

//------------------------------------------------------------------------------
message::message(byte* data, std::size_t size)
    : _data(data)
    , _size(size)
    , _bits_read(0)
    , _bits_written(0)
    , _bytes_read(0)
    , _bytes_written(0)
    , _bytes_reserved(0)
{}

//------------------------------------------------------------------------------
void message::reset()
{
    _bits_read = 0;
    _bits_written = 0;
    _bytes_read = 0;
    _bytes_written = 0;
    _bytes_reserved = 0;
}

//------------------------------------------------------------------------------
void message::rewind() const
{
    _bits_read = 0;
    _bytes_read = 0;
}

//------------------------------------------------------------------------------
void message::rewind(std::size_t size) const
{
    if (_bytes_read < size) {
        _bytes_read = 0;
    } else {
        _bytes_read -= size;
    }
}

//------------------------------------------------------------------------------
void message::rewind_bits(std::size_t bits) const
{
    if (_bytes_read * byte_bits + _bits_read < bits) {
        _bits_read = 0;
        _bytes_read = 0;
    } else {
        _bytes_read -= bits / byte_bits;
        if (_bits_read && (bits % byte_bits) >= _bits_read) {
            --_bytes_read;
        }
        _bits_read += byte_bits - (bits % byte_bits);
        _bits_read %= byte_bits;
    }
}

//------------------------------------------------------------------------------
byte* message::write(std::size_t size)
{
    if (_bytes_reserved) {
        return nullptr;
    } else if (_bytes_written + size > _size) {
        return nullptr;
    }

    _bits_written = 0;
    _bytes_written += size;
    return _data + _bytes_written - size;
}

//------------------------------------------------------------------------------
byte const* message::read(std::size_t size) const
{
    if (_bytes_read + size > _bytes_written) {
        return nullptr;
    }

    _bits_read = 0;
    _bytes_read += size;
    return _data + _bytes_read - size;
}

//------------------------------------------------------------------------------
byte* message::reserve(std::size_t size)
{
    if (_bytes_written + _bytes_reserved + size > _size) {
        return nullptr;
    }

    _bits_written = 0;
    _bytes_reserved += size;
    return _data + _bytes_written + _bytes_reserved - size;
}

//------------------------------------------------------------------------------
std::size_t message::commit(std::size_t size)
{
    if (_bytes_reserved < size) {
        _bytes_reserved = 0;
        return 0;
    }

    _bytes_written += size;
    _bytes_reserved = 0;
    return size;
}

//------------------------------------------------------------------------------
std::size_t message::write(byte const* data, std::size_t size)
{
    if (_bytes_written + size > _size) {
        return 0;
    }

    memcpy(_data + _bytes_written, data, size);
    _bits_written = 0;
    _bytes_written += size;
    return size;
}

//------------------------------------------------------------------------------
std::size_t message::read(byte* data, std::size_t size) const
{
    if (_bytes_read + size > _bytes_written) {
        return 0;
    }

    memcpy(data, _data + _bytes_read, size);
    _bits_read = 0;
    _bytes_read += size;
    return size;
}

//------------------------------------------------------------------------------
std::size_t message::write(network::message const& message)
{
    std::size_t remaining = message.bytes_remaining();
    return write(message.read(remaining), remaining);
}

//------------------------------------------------------------------------------
std::size_t message::read(network::message& message) const
{
    std::size_t remaining = bytes_remaining();
    return message.write(read(remaining), remaining);
}

//------------------------------------------------------------------------------
std::size_t message::bits_read() const
{
    return _bytes_read * byte_bits - ((byte_bits - _bits_read) % byte_bits);
}

//------------------------------------------------------------------------------
std::size_t message::bits_written() const
{
    return _bytes_written * byte_bits - ((byte_bits - _bits_written) % byte_bits);
}

//------------------------------------------------------------------------------
std::size_t message::bits_remaining() const
{
    return bits_written() - bits_read();
}

//------------------------------------------------------------------------------
std::size_t message::bits_available() const
{
    return bytes_available() * byte_bits + ((byte_bits - _bits_written) % byte_bits);
}

//------------------------------------------------------------------------------
void message::write_bits(int value, int bits)
{
    // `bits` can be negative for symmetry with read_bits() but
    // it doesn't affect bit representation in the output buffer.
    int value_bits = (bits < 0) ? -bits : bits;

    // can't write if bytes are reserved
    if (_bytes_reserved) {
        return;
    }

    // check for overflow
    if (bits_available() < value_bits) {
        return;
    }

    while(value_bits) {
        // advance write cursor
        if (_bits_written == 0) {
            _data[_bytes_written++] = 0;
        }

        // number of bits to write this step
        int put = std::min<int>(byte_bits - _bits_written, value_bits);

        // extract bits from value
        int masked = value & ((1 << put) - 1);
        // insert bits into buffer
        _data[_bytes_written - 1] |= masked << _bits_written;

        value_bits -= put;
        value >>= put;
        _bits_written = (_bits_written + put) % byte_bits;
    }
}

//------------------------------------------------------------------------------
void message::write_align()
{
    _bits_written = 0;
}

//------------------------------------------------------------------------------
void message::write_float(float f)
{
    int i;

    memcpy(&i, &f, sizeof(i));
    write_bits(i, 32);
}

//------------------------------------------------------------------------------
void message::write_vector(vec2 v)
{
    write_float(v.x);
    write_float(v.y);
}

//------------------------------------------------------------------------------
void message::write_string(char const* sz)
{
    if (sz) {
        write((byte const*)sz, strlen(sz) + 1);
    } else {
        write((byte const*)"", 1);
    }
}

//------------------------------------------------------------------------------
int message::read_bits(int bits) const
{
    // `bits` can be negative to indicate that the value should be sign-extended
    int total_bits = (bits < 0) ? -bits : bits;
    int value_bits = total_bits;
    int value = 0;

    // check for underflow
    if (bits_remaining() < value_bits) {
        return -1;
    }

    while (value_bits) {
        // advance read cursor
        if (_bits_read == 0) {
            _bytes_read++;
        }

        // number of bits to read this step
        int get = std::min<int>(byte_bits - _bits_read, value_bits);

        // extract bits from buffer
        int masked = _data[_bytes_read - 1];
        masked >>= _bits_read;
        masked &= ((1 << get) - 1);
        // insert bits into value
        value |= masked << (total_bits - value_bits);

        value_bits -= get;
        _bits_read = (_bits_read + get) % byte_bits;
    }

    // sign extend value if original `bits` was negative
    if (bits < 0 && value & (1 << (-bits - 1))) {
        value |= -1 ^ ((1 << -bits) - 1);
    }

    return value;
}

//------------------------------------------------------------------------------
void message::read_align() const
{
    _bits_read = 0;
}

//------------------------------------------------------------------------------
float message::read_float() const
{
    float f;

    int i = read_bits(32);
    memcpy(&f, &i, sizeof(i));
    return f;
}

//------------------------------------------------------------------------------
vec2 message::read_vector() const
{
    if (_bytes_read + 8 > _bytes_written) {
        return vec2_zero;
    }

    float outx = read_float();
    float outy = read_float();

    return vec2(outx, outy);
}

//------------------------------------------------------------------------------
char const* message::read_string() const
{
    std::size_t remaining = bytes_remaining();
    char const* str = (char const* )read(remaining);
    std::size_t len = strnlen(str, remaining);

    rewind(remaining - len - 1);
    return (char const*)str;
}

//------------------------------------------------------------------------------
void delta_message::write_bits(int value, int bits)
{
    if (_target) {
        _target->write_bits(value, bits);
    }

    if (!_source) {
        _writer->write_bits(value, bits);
    } else if (value == _source->read_bits(bits)) {
        _writer->write_bits(0, 1);
    } else {
        _writer->write_bits(1, 1);
        _writer->write_bits(value, bits);
    }
}

//------------------------------------------------------------------------------
int delta_message::read_bits(int bits) const
{
    int value;

    if (!_source) {
        value = _reader->read_bits(bits);
    } else {
        int base_value = _source->read_bits(bits);
        if (!_reader->read_bits(1)) {
            value = base_value;
        } else {
            value = _reader->read_bits(bits);
        }
    }

    if (_target) {
        _target->write_bits(value, bits);
    }

    return value;
}

} // namespace network
