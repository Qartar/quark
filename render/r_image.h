// r_image.h
//

#pragma once

#include "cm_error.h"
#include "cm_string.h"

#include "gl/gl_texture.h"

typedef struct HBITMAP__* HBITMAP;

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
enum class image_format
{
    bgr8,
    rgba8,
    rgb10a2,
};

//------------------------------------------------------------------------------
class image
{
public:
    image(string::view name);
    ~image();

    string::view name() const { return _name; }
    gl::texture const& texture() const { return _texture; }
    int width() const { return _width; }
    int height() const { return _height; }

    static result write_bmp(string::view filename, int width, int height, image_format format, uint8_t const* data);
    static result write_dds(string::view filename, int width, int height, image_format format, uint8_t const* data, bool use_legacy_encoding = true);

protected:
    string::buffer _name;
    gl::texture2d _texture;
    int _width;
    int _height;

protected:
    HBITMAP load_bitmap_from_resource(string::view name) const;
    HBITMAP load_bitmap_from_file(string::view name) const;

    result load_file(string::view filename);

    result read_dds(string::view filename, int& width, int& height, image_format& format, std::vector<uint8_t>& data) const;

    bool upload(HBITMAP bitmap);
};

} // namespace render
