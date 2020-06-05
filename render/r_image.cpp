//  r_image.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_filesystem.h"
#include "r_image.h"
#include "gl/gl_include.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

namespace {

//------------------------------------------------------------------------------
constexpr GLenum gl_internal_format(image_format format)
{
    switch (format) {
        case image_format::bgr8:
            return GL_RGB8;
        case image_format::rgba8:
            return GL_RGBA8;
        case image_format::rgb10a2:
            return GL_RGB10_A2;
    }
    return 0;
}

//------------------------------------------------------------------------------
constexpr GLenum gl_format(image_format format)
{
    switch (format) {
        case image_format::bgr8:
            return GL_BGR;
        case image_format::rgba8:
        case image_format::rgb10a2:
            return GL_RGBA;
    }
    return 0;
}

//------------------------------------------------------------------------------
constexpr GLenum gl_type(image_format format)
{
    switch (format) {
        case image_format::bgr8:
        case image_format::rgba8:
            return GL_UNSIGNED_BYTE;
        case image_format::rgb10a2:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
    }
    return 0;
}

} // anonymous namespace

//------------------------------------------------------------------------------
render::image const* system::load_image(string::view name)
{
    _images.push_back(std::make_unique<render::image>(name));
    return _images.back().get();
}

//------------------------------------------------------------------------------
void system::draw_image(render::image const* img, vec2 org, vec2 sz, color4 color)
{
    if (img == nullptr) {
        return;
    }

    glEnable(GL_TEXTURE_2D);
    img->texture().bind();

    glColor4fv(color);

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(org.x, org.y);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(org.x + sz.x, org.y);

        glTexCoord2f(0.0f, 1.0f );
        glVertex2f(org.x, org.y + sz.y);

        glTexCoord2f(1.0f, 1.0f );
        glVertex2f(org.x + sz.x, org.y + sz.y);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

//------------------------------------------------------------------------------
image::image(string::view name)
    : _width(0)
    , _height(0)
{
    if (succeeded(load_file(name))) {
        return;
    }

    HBITMAP bitmap = NULL;

    if ((bitmap = load_bitmap_from_resource(name))) {
        _name = string::buffer(va("<resource#%d>", (ULONG_PTR)name.begin()));
    } else if ((bitmap = load_bitmap_from_file(name))) {
        _name = string::buffer(name);
    } else {
        _name = string::buffer("<default>");
    }

    upload(bitmap);

    if (bitmap) {
        DeleteObject(bitmap);
    }
}

//------------------------------------------------------------------------------
image::~image()
{
}

//------------------------------------------------------------------------------
HBITMAP image::load_bitmap_from_resource(string::view name) const
{
    UINT flags = LR_CREATEDIBSECTION;

    return (HBITMAP )LoadImageA(
        application::singleton()->hinstance(), // hinst
        name.begin(),                   // name
        IMAGE_BITMAP,                   // type
        0,                              // cx
        0,                              // cy
        flags                           // fuLoad
    );
}

//------------------------------------------------------------------------------
HBITMAP image::load_bitmap_from_file(string::view name) const
{
    UINT flags = LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE;

    return (HBITMAP )LoadImageA(
        NULL,                           // hinst
        name.c_str(),                   // name
        IMAGE_BITMAP,                   // type
        0,                              // cx
        0,                              // cy
        flags                           // fuLoad
    );
}

//------------------------------------------------------------------------------
bool image::upload(HBITMAP bitmap)
{
    if (!bitmap) {
        return false;
    }

    BITMAP bm;

    if (!GetObjectA(bitmap, sizeof(bm), &bm)) {
        return false;
    }

    std::vector<uint8_t> buffer(bm.bmWidthBytes * bm.bmHeight);

    if (!GetBitmapBits(bitmap, narrow_cast<LONG>(buffer.size()), buffer.data())) {
        return false;
    }

    _width = bm.bmWidth;
    _height = bm.bmHeight;

    _texture = gl::texture2d(1, GL_RGB8, _width, _height);
    _texture.upload(0, 0, 0, _width, _height, GL_BGR, GL_UNSIGNED_BYTE, buffer.data());

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return true;
}

//------------------------------------------------------------------------------
result image::load_file(string::view filename)
{
    image_format format;
    std::vector<uint8_t> data;

    if (succeeded(read_dds(filename, _width, _height, format, data))) {
        _texture = gl::texture2d(1, gl_internal_format(format), _width, _height);
        _texture.upload(0, 0, 0, _width, _height, gl_format(format), gl_type(format), data.data());
        return result::success;
    } else {
        return result::failure;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Bitmap (BMP)

//------------------------------------------------------------------------------
result image::write_bmp(string::view filename, int width, int height, image_format format, uint8_t const* data)
{
    if (format != image_format::bgr8) {
        log::warning("failed to write image '^fff%.*s^xxx', unsupported format\n", filename.length(), filename.begin());
        return result::failure;
    }

    BITMAPINFOHEADER bmi{};
    bmi.biSize = sizeof(bmi);
    bmi.biWidth = width;
    bmi.biHeight = height;
    bmi.biPlanes = 1;
    bmi.biCompression = BI_RGB;
    bmi.biBitCount = 24;
    bmi.biSizeImage = 0;
    bmi.biXPelsPerMeter = 0;
    bmi.biYPelsPerMeter = 0;
    bmi.biClrUsed = 0;
    bmi.biClrImportant = 0;

    BITMAPFILEHEADER hdr{};

    hdr.bfType = 0x4d42;
    hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) + width * height * 3;
    hdr.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

    if (auto f = file::open(filename, file::mode::write)) {
        f.write((file::byte const*)&hdr, sizeof(hdr));
        f.write((file::byte const*)&bmi, sizeof(bmi));
        f.write(data, width * height * 3);
        f.close();
    } else {
        log::warning("failed to write image '^fff%.*s^xxx'\n", filename.length(), filename.begin());
        return result::failure;
    }

    return result::success;
}

////////////////////////////////////////////////////////////////////////////////
// DirectDraw Surface (DDS)

//------------------------------------------------------------------------------
struct dds_pixelformat {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

struct dds_header {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    dds_pixelformat ddspf;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};

struct dds_header_dx10 {
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlag2;
};

static_assert(sizeof(dds_header) == 124);
static_assert(sizeof(dds_pixelformat) == 32);

constexpr uint32_t DDS_MAGIC = ('D' << 0 | 'D' << 8 | 'S' << 16 | ' ' << 24);
constexpr int DDSD_CAPS = 0x1;
constexpr int DDSD_HEIGHT = 0x2;
constexpr int DDSD_WIDTH = 0x4;
constexpr int DDSD_PITCH = 0x8;
constexpr int DDSD_PIXELFORMAT = 0x1000;
constexpr int DDPF_ALPHAPIXELS = 0x1;
constexpr int DDPF_FOURCC = 0x4;
constexpr int DDPF_RGB = 0x40;
constexpr uint32_t FOURCC_DX10 = ('D' << 0 | 'X' << 8 | '1' << 16 | '0' << 24);
constexpr int DDSCAPS_TEXTURE = 0x1000;
constexpr int DXGI_FORMAT_R10G10B10A2_UNORM = 24;
constexpr int DXGI_FORMAT_R8G8B8A8_UNORM = 28;
constexpr int DDS_DIMENSION_TEXTURE2D  = 3;

//------------------------------------------------------------------------------
result image::write_dds(string::view filename, int width, int height, image_format format, uint8_t const* data, bool use_legacy_encoding)
{
    dds_header header{};
    header.dwSize = sizeof(dds_header);
    header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT;
    header.dwHeight = height;
    header.dwWidth = width;
    header.dwDepth = 1; // not required
    header.dwMipMapCount = 1; // not required
    header.dwPitchOrLinearSize = width * sizeof(uint32_t);
    header.ddspf.dwSize = sizeof(dds_pixelformat);
    header.dwCaps = DDSCAPS_TEXTURE;

    // Legacy encoding for tools that don't support DDS_HEADER_DX10
    if (use_legacy_encoding) {
        switch (format) {
            case image_format::rgba8:
                header.ddspf.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
                header.ddspf.dwRGBBitCount = 32;
                header.ddspf.dwRBitMask = 0x000000ff;
                header.ddspf.dwGBitMask = 0x0000ff00;
                header.ddspf.dwBBitMask = 0x00ff0000;
                header.ddspf.dwABitMask = 0xff000000;
                break;

            case image_format::rgb10a2:
                header.ddspf.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
                header.ddspf.dwRGBBitCount = 32;
                header.ddspf.dwRBitMask = 0x000003ff;
                header.ddspf.dwGBitMask = 0x000ffc00;
                header.ddspf.dwBBitMask = 0x3ff00000;
                header.ddspf.dwABitMask = 0xc0000000;
                break;

            default:
                log::warning("failed to write image '^fff%.*s^xxx', unsupported format\n", filename.length(), filename.begin());
                return result::failure;
        }

        if (auto f = file::open(filename, file::mode::write)) {
            f.write((file::byte const*)&DDS_MAGIC, sizeof(DDS_MAGIC));
            f.write((file::byte const*)&header, sizeof(header));
            f.write((file::byte const*)data, height * header.dwPitchOrLinearSize);
            f.close();
        } else {
            log::warning("failed to write image '^fff%.*s^xxx'\n", filename.length(), filename.begin());
            return result::failure;
        }
    } else {
        header.ddspf.dwFlags = DDPF_FOURCC;
        header.ddspf.dwFourCC = FOURCC_DX10;

        dds_header_dx10 header10{};
        switch (format) {
            case image_format::rgba8:
                header10.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;

            case image_format::rgb10a2:
                header10.dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
                break;

            default:
                return result::failure;
        }
        header10.resourceDimension = DDS_DIMENSION_TEXTURE2D;

        if (auto f = file::open(filename, file::mode::write)) {
            f.write((file::byte const*)&DDS_MAGIC, sizeof(DDS_MAGIC));
            f.write((file::byte const*)&header, sizeof(header));
            f.write((file::byte const*)&header10, sizeof(header10));
            f.write((file::byte const*)data, height * header.dwPitchOrLinearSize);
            f.close();
        } else {
            log::warning("failed to write image '^fff%.*s^xxx'\n", filename.length(), filename.begin());
            return result::failure;
        }
    }

    return result::success;
}

//------------------------------------------------------------------------------
result image::read_dds(string::view filename, int& width, int& height, image_format& format, std::vector<uint8_t>& data) const
{
    auto f = file::read(filename);
    if (f.size() < sizeof(uint32_t) + sizeof(dds_header)
        || *reinterpret_cast<uint32_t const*>(f.data()) != DDS_MAGIC) {
        log::warning("failed to read image '^fff%.*s^xxx', invalid header\n", filename.length(), filename.begin());
        return result::failure;
    }

    dds_header const* header = reinterpret_cast<dds_header const*>(f.data() + 4);
    if (header->ddspf.dwFlags & DDPF_FOURCC && header->ddspf.dwFourCC == FOURCC_DX10) {
        if (f.size() < sizeof(uint32_t) + sizeof(dds_header) + sizeof(dds_header_dx10)) {
            log::warning("failed to read image '^fff%.*s^xxx', invalid header\n", filename.length(), filename.begin());
            return result::failure;
        }

        dds_header_dx10 const* header10 = reinterpret_cast<dds_header_dx10 const*>(&header[1]);
        if (header10->dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM
            && header10->resourceDimension == DDS_DIMENSION_TEXTURE2D) {
            format = image_format::rgba8;
        } else if (header10->dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM
            && header10->resourceDimension == DDS_DIMENSION_TEXTURE2D) {
            format = image_format::rgb10a2;
        } else {
            log::warning("failed to read image '^fff%.*s^xxx', unsupported format\n", filename.length(), filename.begin());
            return result::failure;
        }

        width = header->dwWidth;
        height = header->dwHeight;
        data.resize(header->dwHeight * header->dwPitchOrLinearSize);
        if (f.size() < sizeof(uint32_t) + sizeof(dds_header) + sizeof(dds_header_dx10) + data.size()) {
            log::warning("failed to read image '^fff%.*s^xxx', insufficient data\n", filename.length(), filename.begin());
            return result::failure;
        } else {
            memcpy(data.data(), reinterpret_cast<std::byte const*>(&header10[1]), data.size());
        }

    } else {
        if (header->ddspf.dwFlags == (DDPF_ALPHAPIXELS | DDPF_RGB)
            && header->ddspf.dwRGBBitCount == 32
            && header->ddspf.dwRBitMask == 0x000000ff
            && header->ddspf.dwGBitMask == 0x0000ff00
            && header->ddspf.dwBBitMask == 0x00ff0000
            && header->ddspf.dwABitMask == 0xff000000) {
            format = image_format::rgba8;
        } else if (header->ddspf.dwFlags == (DDPF_ALPHAPIXELS | DDPF_RGB)
            && header->ddspf.dwRGBBitCount == 32
            && header->ddspf.dwRBitMask == 0x000003ff
            && header->ddspf.dwGBitMask == 0x000ffc00
            && header->ddspf.dwBBitMask == 0x3ff00000
            && header->ddspf.dwABitMask == 0xc0000000) {
            format = image_format::rgb10a2;
        } else {
            log::warning("failed to read image '^fff%.*s^xxx', unsupported format\n", filename.length(), filename.begin());
            return result::failure;
        }

        width = header->dwWidth;
        height = header->dwHeight;
        data.resize(header->dwHeight * header->dwPitchOrLinearSize);
        if (f.size() < sizeof(uint32_t) + sizeof(dds_header) + data.size()) {
            log::warning("failed to read image '^fff%.*s^xxx', insufficient data\n", filename.length(), filename.begin());
            return result::failure;
        } else {
            memcpy(data.data(), reinterpret_cast<std::byte const*>(&header[1]), data.size());
        }

    }

    return result::success;
}

} // namespace render
