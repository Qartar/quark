// r_image.h
//

#pragma once

#include "cm_string.h"

#include "gl/gl_texture.h"

#if defined(_WIN32)
typedef struct HBITMAP__* HBITMAP;
#endif // defined(_WIN32)

////////////////////////////////////////////////////////////////////////////////
namespace render {

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

protected:
    string::buffer _name;
    gl::texture2d _texture;
    int _width;
    int _height;

protected:
    HBITMAP load_resource(string::view name) const;
    HBITMAP load_file(string::view name) const;

    bool upload(HBITMAP bitmap);
};

} // namespace render
