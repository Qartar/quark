// net_msg.inl
//

////////////////////////////////////////////////////////////////////////////////
namespace network {

//------------------------------------------------------------------------------
template<typename impl>
message_base<impl>::message_base(byte* data, std::size_t size)
    : _data(data)
    , _size(size)
    , _bits_read(0)
    , _bits_written(0)
    , _bytes_read(0)
    , _bytes_written(0)
    , _bytes_reserved(0)
{}

//------------------------------------------------------------------------------
template<typename impl>
void message_base<impl>::reset()
{
    _bits_read = 0;
    _bits_written = 0;
    _bytes_read = 0;
    _bytes_written = 0;
    _bytes_reserved = 0;
}

//------------------------------------------------------------------------------
template<typename impl>
void message_base<impl>::rewind() const
{
    _bits_read = 0;
    _bytes_read = 0;
}

//------------------------------------------------------------------------------
template<typename impl>
void message_base<impl>::rewind(std::size_t size) const
{
    if (_bytes_read < size) {
        _bytes_read = 0;
    } else {
        _bytes_read -= size;
    }
}

//------------------------------------------------------------------------------
template<typename impl>
void message_base<impl>::rewind_bits(std::size_t bits) const
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
template<typename impl>
byte* message_base<impl>::write(std::size_t size)
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
template<typename impl>
byte const* message_base<impl>::read(std::size_t size) const
{
    if (_bytes_read + size > _bytes_written) {
        return nullptr;
    }

    _bits_read = 0;
    _bytes_read += size;
    return _data + _bytes_read - size;
}

//------------------------------------------------------------------------------
template<typename impl>
byte* message_base<impl>::reserve(std::size_t size)
{
    if (_bytes_written + _bytes_reserved + size > _size) {
        return nullptr;
    }

    _bits_written = 0;
    _bytes_reserved += size;
    return _data + _bytes_written + _bytes_reserved - size;
}

//------------------------------------------------------------------------------
template<typename impl>
std::size_t message_base<impl>::commit(std::size_t size)
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
template<typename impl>
std::size_t message_base<impl>::write(byte const* data, std::size_t size)
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
template<typename impl>
std::size_t message_base<impl>::read(byte* data, std::size_t size) const
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
template<typename impl> template<typename T>
std::size_t message_base<impl>::write(network::message_base<T> const& message)
{
    std::size_t remaining = message.bytes_remaining();
    return write(message.read(remaining), remaining);
}

//------------------------------------------------------------------------------
template<typename impl> template<typename T>
std::size_t message_base<impl>::read(network::message_base<T>& message) const
{
    std::size_t remaining = bytes_remaining();
    return message.write(read(remaining), remaining);
}

//------------------------------------------------------------------------------
template<typename impl>
std::size_t message_base<impl>::bits_read() const
{
    return _bytes_read * byte_bits - ((byte_bits - _bits_read) % byte_bits);
}

//------------------------------------------------------------------------------
template<typename impl>
std::size_t message_base<impl>::bits_written() const
{
    return _bytes_written * byte_bits - ((byte_bits - _bits_written) % byte_bits);
}

//------------------------------------------------------------------------------
template<typename impl>
std::size_t message_base<impl>::bits_remaining() const
{
    return bits_written() - bits_read();
}

//------------------------------------------------------------------------------
template<typename impl>
std::size_t message_base<impl>::bits_available() const
{
    return bytes_available() * byte_bits + ((byte_bits - _bits_written) % byte_bits);
}

//------------------------------------------------------------------------------
template<typename impl>
void message_base<impl>::write_align()
{
    _bits_written = 0;
}

//------------------------------------------------------------------------------
template<typename impl>
void message_base<impl>::write_float(float f)
{
    int i;

    memcpy(&i, &f, sizeof(i));
    static_cast<typename impl*>(this)->write_bits(i, 32);
}

//------------------------------------------------------------------------------
template<typename impl>
void message_base<impl>::write_vector(vec2 v)
{
    write_float(v.x);
    write_float(v.y);
}

//------------------------------------------------------------------------------
template<typename impl>
void message_base<impl>::write_string(char const* sz)
{
    if (sz) {
        write((byte const*)sz, strlen(sz) + 1);
    } else {
        write((byte const*)"", 1);
    }
}

//------------------------------------------------------------------------------
template<typename impl>
void message_base<impl>::read_align() const
{
    _bits_read = 0;
}

//------------------------------------------------------------------------------
template<typename impl>
float message_base<impl>::read_float() const
{
    float f;

    int i = static_cast<typename impl const*>(this)->read_bits(32);
    memcpy(&f, &i, sizeof(i));
    return f;
}

//------------------------------------------------------------------------------
template<typename impl>
vec2 message_base<impl>::read_vector() const
{
    if (_bytes_read + 8 > _bytes_written) {
        return vec2_zero;
    }

    float outx = read_float();
    float outy = read_float();

    return vec2(outx, outy);
}

//------------------------------------------------------------------------------
template<typename impl>
char const* message_base<impl>::read_string() const
{
    std::size_t remaining = bytes_remaining();
    char const* str = (char const* )read(remaining);
    std::size_t len = strnlen(str, remaining);

    rewind(remaining - len - 1);
    return (char const*)str;
}

} // namespace network
