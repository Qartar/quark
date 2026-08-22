// cm_filesystem.h
//

#pragma once

#include "cm_string.h"

#include <cstdint>
#include <cstdio>
#include <io.h>

typedef struct _iobuf FILE;

////////////////////////////////////////////////////////////////////////////////
namespace file {

using byte = std::uint8_t;

//------------------------------------------------------------------------------
enum class seek
{
    set, //!< seek relative to the beginning of the stream
    cur, //!< seek relative to the current position in the stream
    end, //!< seek relative to the end of the stream
};

//------------------------------------------------------------------------------
class stream
{
public:
    stream();
    stream(stream&& other);
    stream& operator=(stream&& other);
    ~stream();

    //! return true if stream is valid
    operator bool() const { return _handle != nullptr; }
    //! close the current stream
    void close();
    //! return the size of the stream
    std::size_t size() const;
    //! seek to the given offset within the stream
    bool seek(std::intptr_t offset, file::seek origin);
    //! return the current offset from the beginning of the stream
    std::intptr_t tell() const;

    //! print formatted string into the stream
    std::size_t printf(string::literal fmt, ...);
    //! write data from the given buffer into the stream
    std::size_t write(byte const* data, std::size_t size);
    //! read data from the stream into the given buffer
    std::size_t read(byte* data, std::size_t size) const;

protected:
    FILE* _handle;

protected:
    stream(FILE* handle);
};

//------------------------------------------------------------------------------
class buffer
{
public:
    buffer();
    buffer(buffer&& other);
    buffer& operator=(buffer&& other);
    ~buffer();

    //! return pointer to internal buffer
    byte const* data() const { return _data; }
    //! return size of internal buffer
    std::size_t size() const { return _size; }

protected:
    byte const* _data;
    std::size_t _size;

protected:
    buffer(byte const* data, std::size_t size);
};

//------------------------------------------------------------------------------
enum class mode
{
    read,
    write,
    append,
};

//------------------------------------------------------------------------------
stream open(string::view filename, file::mode mode);

//------------------------------------------------------------------------------
buffer read(string::view filename);

//------------------------------------------------------------------------------
std::size_t write(string::view filename, byte const* buffer, std::size_t buffer_size);

//------------------------------------------------------------------------------
enum class time : uint64_t {};

//------------------------------------------------------------------------------
time modified_time(string::view filename);

//------------------------------------------------------------------------------
class find
{
public:
    find(string::view path, string::view filter);
    find(find const&) = delete;
    find& operator=(find const&) = delete;
    ~find();

    //! return true if current file is valid
    explicit operator bool() const;
    //! advance to the next file
    find& operator++();
    //! return name of current file including search path
    string::view fullname() const;
    //! return name of current file without search path
    string::view filename() const;

protected:
    string::buffer _path;
    intptr_t _handle;
    __finddata64_t _data;
};

} // namespace file
