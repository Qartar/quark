// cm_filesystem.cpp
//

#include "cm_filesystem.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <utility>

#include <sys/stat.h>

////////////////////////////////////////////////////////////////////////////////
namespace file {

namespace {

//------------------------------------------------------------------------------
constexpr int seek_to_native(file::seek seek)
{
    switch (seek) {
        case seek::set: return SEEK_SET;
        case seek::cur: return SEEK_CUR;
        case seek::end: return SEEK_END;
        default: assert(0); return 0;
    }
}

//------------------------------------------------------------------------------
constexpr char const* mode_to_native(file::mode mode)
{
    switch (mode) {
        case mode::read: return "rb";
        case mode::write: return "wb";
        case mode::append: return "ab";
        default: assert(0); return 0;
    }
}

//------------------------------------------------------------------------------
class stream_internal : public stream
{
public:
    stream_internal(FILE* handle)
        : stream(handle)
    {}
};

//------------------------------------------------------------------------------
class buffer_internal : public buffer
{
public:
    buffer_internal(byte const* data, std::size_t size)
        : buffer(data, size)
    {}
};

} // anonymous namespace

//------------------------------------------------------------------------------
stream::stream()
    : _handle(NULL)
{
}

//------------------------------------------------------------------------------
stream::stream(FILE* handle)
    : _handle(handle)
{
}

//------------------------------------------------------------------------------
stream::stream(stream&& other)
    : _handle(other._handle)
{
    other._handle = NULL;
}

//------------------------------------------------------------------------------
stream& stream::operator=(stream&& other)
{
    if (_handle && &other != this) {
        fclose(_handle);
        _handle = NULL;
    }
    std::swap(_handle, other._handle);
    return *this;
}

//------------------------------------------------------------------------------
stream::~stream()
{
    if (_handle) {
        fclose(_handle);
    }
}

//------------------------------------------------------------------------------
void stream::close()
{
    if (_handle) {
        fclose(_handle);
        _handle = NULL;
    }
}

//------------------------------------------------------------------------------
bool stream::seek(std::intptr_t offset, file::seek origin)
{
#if defined(_WIN32)
    return _fseeki64(_handle, offset, seek_to_native(origin)) == 0;
#else
    return fseek(_handle, offset, seek_to_native(origin)) == 0;
#endif
}

//------------------------------------------------------------------------------
std::size_t stream::size() const
{
    if (!_handle) {
        return 0;
    }

    fpos_t pos;
    fgetpos(_handle, &pos);
#if defined(_WIN32)
    _fseeki64(_handle, 0, SEEK_SET);
    long long start = _ftelli64(_handle);
    _fseeki64(_handle, 0, SEEK_END);
    long long end = _ftelli64(_handle);
#else
    fseek(_handle, 0, SEEK_SET);
    long start = ftell(_handle);
    fseek(_handle, 0, SEEK_END);
    long end = ftell(_handle);
#endif
    fsetpos(_handle, &pos);
    return end - start;
}

//------------------------------------------------------------------------------
std::size_t stream::printf(string::literal fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    int len = vfprintf(_handle, fmt.begin(), ap);
    va_end(ap);

    return len > 0 ? len : 0;
}

//------------------------------------------------------------------------------
std::size_t stream::write(byte const* data, std::size_t size)
{
    return _handle ? fwrite(data, 1, size, _handle) : 0;
}

//------------------------------------------------------------------------------
std::size_t stream::read(byte* data, std::size_t size) const
{
    return _handle ? fread(data, 1, size, _handle) : 0;
}

//------------------------------------------------------------------------------
buffer::buffer()
    : _data(nullptr)
    , _size(0)
{
}

//------------------------------------------------------------------------------
buffer::buffer(byte const* data, std::size_t size)
    : _data(data)
    , _size(size)
{
}

//------------------------------------------------------------------------------
buffer::buffer(buffer&& other)
    : _data(other._data)
    , _size(other._size)
{
    other._data = nullptr;
    other._size = 0;
}

//------------------------------------------------------------------------------
buffer& buffer::operator=(buffer&& other)
{
    if (_data && &other != this) {
        delete [] _data;
        _size = 0;
    }
    std::swap(_data, other._data);
    std::swap(_size, other._size);
    return *this;
}

//------------------------------------------------------------------------------
buffer::~buffer()
{
    delete [] _data;
}

//------------------------------------------------------------------------------
stream open(string::view filename, file::mode mode)
{
    FILE* f = nullptr;
    fopen_s(&f, filename.c_str(), mode_to_native(mode));
    return stream_internal(f);
}

//------------------------------------------------------------------------------
buffer read(string::view filename)
{
    stream f = open(filename, mode::read);
    if (f) {
        std::size_t size = f.size();
        byte* data = new byte[size];
        std::size_t bytes_read = f.read(data, size);
        assert(bytes_read == size);
        /*unreferenced variable*/(void)bytes_read;
        return buffer_internal(data, size);
    } else {
        return buffer();
    }
}

//------------------------------------------------------------------------------
std::size_t write(string::view filename, byte const* buffer, std::size_t buffer_size)
{
    stream f = open(filename, mode::write);
    return f.write(buffer, buffer_size);
}

//------------------------------------------------------------------------------
time modified_time(string::view filename)
{
#if defined(_WIN32)
    struct _stat64 s;
    if (!_stat64(filename.c_str(), &s)) {
        return static_cast<file::time>(s.st_mtime);
    } else {
        return static_cast<file::time>(0);
    }
#endif
}

} // namespace file
