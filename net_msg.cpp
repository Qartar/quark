// net_msg.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "net_main.h"

////////////////////////////////////////////////////////////////////////////////
namespace network {

//------------------------------------------------------------------------------
message::message(byte* data, std::size_t size)
    : message_base(data, size)
{}

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
delta_message::delta_message(network::message const* source, network::message const* reader, network::message* target)
    : _source(source)
    , _reader(reader)
    , _writer(nullptr)
    , _target(target)
{}

//------------------------------------------------------------------------------
delta_message::delta_message(network::message const* source, network::message* writer, network::message* target)
    : _source(source)
    , _reader(nullptr)
    , _writer(writer)
    , _target(target)
{}

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
